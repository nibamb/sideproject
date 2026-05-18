/*
 * ACHROMATOSIS OS - Main System Entry Point
 * 
 * Bezpieczna inicjalizacja ESP32-S3 z:
 * - Waveshare 3.49" Touch LCD (172x640, AXS15231B na QSPI)
 * - LVGL v9 z buforem w PSRAM
 * - USB CDC Serial Monitor (opóźniona inicjalizacja)
 * - Error-proof memory management i kontrola błędów sprzętu
 */

#include <Arduino.h>
#include <lvgl.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

extern "C" {
    #include "esp_lcd_axs15231b.h"
    #include "driver/gpio.h"
    #include "driver/spi_master.h"
    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_panel_vendor.h"
    #include "esp_lcd_panel_ops.h"
}

#include "ui_launcher.h"
#include "achromatosis_config.h"
#include "system_i2c.h"
#include "system_touch.h"
#include "app_player_audio.h"

// ============================================================================
// ZDEFINIOWANE STAŁE
// ============================================================================

// Bufor renderowania: FULL FRAME BUFFER dla prawidłowego wyświetlania
// PSRAM ma 8MB, więc możemy sobie pozwolić na full buffer
// 172 px (szer.) * 640 px (wys.) * 2 bajtów = 220,160 bajtów (OK dla PSRAM)
#define DRAW_BUF_HEIGHT        640
#define DRAW_BUF_SIZE          (DISPLAY_WIDTH * DRAW_BUF_HEIGHT)
#define DRAW_BUF_BYTES         (DRAW_BUF_SIZE * sizeof(uint16_t))

#define LVGL_TICK_PERIOD_MS    5
#define STARTUP_LOG_DELAY_MS   3000

#define ACHROM_UI_TEST_STAGE2_MS   1000
#define ACHROM_UI_TEST_STAGE3_MS   2500
#define ACHROM_UI_TEST_STAGE4_MS   3000

// Tagi logowania
static const char * TAG_MAIN = "ACHROMATOSIS_MAIN";
static const char * TAG_LCD = "LCD_DRIVER";
static const char * TAG_MEM = "MEMORY_MGR";

// ============================================================================
// ZMIENNE GLOBALNE - Stan systemu
// ============================================================================

static lv_display_t *lv_disp = NULL;
static lv_indev_t *lv_touch_indev = NULL;
static uint16_t *buf1 = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

static uint32_t s_ui_test_t0_ms = 0;
static bool s_ui_test_armed = false;
static bool s_ui_test_s1 = false;
static bool s_ui_test_s2 = false;
static bool s_ui_test_s3 = false;
static bool s_ui_test_s4 = false;
static uint32_t s_lv_tick_last_ms = 0;

static struct {
    bool usb_cdc_ready;
    bool panel_initialized;
    bool lvgl_initialized;
    bool launcher_ready;
} system_state = {
    .usb_cdc_ready = false,
    .panel_initialized = false,
    .lvgl_initialized = false,
    .launcher_ready = false
};

// ============================================================================
// FUNKCJA CALLBACK - Flush LVGL do panelu LCD
// ============================================================================

static void disp_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    if (!panel_handle) {
        if (system_state.usb_cdc_ready) {
            Serial.println("[ERROR] Panel handle is NULL in flush callback!");
        }
        lv_display_flush_ready(disp);
        return;
    }

    // Współrzędne z LVGL - area->x2, area->y2 to OSTATNI PIXEL (inclusive)
    // esp_lcd_panel_draw_bitmap() oczekuje (x_start, y_start, x_end, y_end)
    // gdzie x_end i y_end to EXCLUSIVE (o 1 więcej niż ostatni pixel)
    int offsetx1 = area->x1;
    int offsetx2 = area->x2 + 1;  // +1 bo area->x2 to indeks, nie rozmiar
    int offsety1 = area->y1;
    int offsety2 = area->y2 + 1;  // +1 bo area->y2 to indeks, nie rozmiar

    esp_err_t ret = esp_lcd_panel_draw_bitmap(
        panel_handle, 
        offsetx1, offsety1, 
        offsetx2, offsety2, 
        px_map
    );

    if (ret != ESP_OK && system_state.usb_cdc_ready) {
        Serial.printf("[ERROR] esp_lcd_panel_draw_bitmap failed: %s (coords: %d,%d -> %d,%d)\n", 
                      esp_err_to_name(ret), offsetx1, offsety1, offsetx2, offsety2);
    }

    lv_display_flush_ready(disp);
}

// ============================================================================
// CALLBACK - Aplikacja wybrana z Launchera
// ============================================================================

static void on_app_selected(int app_id) {
    if (system_state.usb_cdc_ready) {
        Serial.printf("[LAUNCHER] Selected application ID: %d\n", app_id);
    }
}

static void touch_indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    system_touch_point_t point = {};
    if (system_touch_get_last(&point)) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = point.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_tick_update(void)
{
    const uint32_t now = millis();
    const uint32_t elapsed = now - s_lv_tick_last_ms;
    if (elapsed > 0) {
        lv_tick_inc(elapsed);
        s_lv_tick_last_ms = now;
    }
}

static void paint_black_screen(void)
{
    lv_obj_t *const scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_invalidate(scr);
}

static void run_startup_ui_test(uint32_t now_ms)
{
    if (!s_ui_test_armed) {
        return;
    }

    const uint32_t elapsed = now_ms - s_ui_test_t0_ms;

    if (!s_ui_test_s1) {
        s_ui_test_s1 = true;
        gpio_set_level(GPIO_NUM_8, 1);
        paint_black_screen();
        Serial.println("[TEST] Hardware up. Initializing Neumorphic Launcher...");
        lv_timer_handler();
    }

    if (!s_ui_test_s2 && elapsed >= ACHROM_UI_TEST_STAGE2_MS) {
        s_ui_test_s2 = true;
        ui_launcher_show();
        Serial.println("[TEST] Layout drawn. Ready for interaction.");
        lv_timer_handler();
    }

    if (!s_ui_test_s3 && elapsed >= ACHROM_UI_TEST_STAGE3_MS) {
        s_ui_test_s3 = true;
        ui_launcher_simulate_row_press(1, true);
        Serial.println("[TEST] Simulating click on 'Snow' (RSVP)...");
        lv_timer_handler();
    }

    if (!s_ui_test_s4 && elapsed >= ACHROM_UI_TEST_STAGE4_MS) {
        s_ui_test_s4 = true;
        ui_launcher_simulate_row_press(1, false);
        ui_launcher_dispatch_row(1);
        Serial.println("[TEST] Dispatched App ID: APP_CHROMA_RSVP. System Stable.");
        lv_timer_handler();
    }
}

// ============================================================================
// HARDWARE SETUP - Inicjalizacja fizycznych urządzeń
// ============================================================================

static esp_err_t setup_backlight_and_reset(void) {
    esp_err_t ret = ESP_OK;

    // Podświetlenie (GPIO 8)
    gpio_config_t bl_config = {};
    bl_config.pin_bit_mask = (1ULL << GPIO_NUM_8);
    bl_config.mode = GPIO_MODE_OUTPUT;
    bl_config.pull_up_en = GPIO_PULLUP_DISABLE;
    bl_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    bl_config.intr_type = GPIO_INTR_DISABLE;
    ret = gpio_config(&bl_config);
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to configure backlight GPIO: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
        return ret;
    }
    gpio_set_level(GPIO_NUM_8, 1);

    // Reset ekranu (GPIO 21)
    gpio_config_t rst_config = {};
    rst_config.pin_bit_mask = (1ULL << GPIO_NUM_21);
    rst_config.mode = GPIO_MODE_OUTPUT;
    rst_config.pull_up_en = GPIO_PULLUP_DISABLE;
    rst_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    rst_config.intr_type = GPIO_INTR_DISABLE;
    ret = gpio_config(&rst_config);
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to configure reset GPIO: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
        return ret;
    }

    // Sekwencja resetu: HIGH -> LOW (250ms) -> HIGH (50ms)
    gpio_set_level(GPIO_NUM_21, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(GPIO_NUM_21, 0);
    vTaskDelay(pdMS_TO_TICKS(250));
    gpio_set_level(GPIO_NUM_21, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    if (system_state.usb_cdc_ready) {
        Serial.printf("[%s] Backlight and reset initialized successfully\n", TAG_LCD);
    }
    return ESP_OK;
}

static esp_err_t setup_spi_bus(void) {
    esp_err_t ret = ESP_OK;

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = GPIO_NUM_11;
    buscfg.miso_io_num = GPIO_NUM_12;
    buscfg.sclk_io_num = GPIO_NUM_10;
    buscfg.quadwp_io_num = GPIO_NUM_13;
    buscfg.quadhd_io_num = GPIO_NUM_14;
    buscfg.max_transfer_sz = DRAW_BUF_SIZE * sizeof(uint16_t);

    ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to initialize SPI bus: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
        return ret;
    }

    if (system_state.usb_cdc_ready) {
        Serial.printf("[%s] SPI3 bus initialized at 40 MHz QSPI mode\n", TAG_LCD);
    }
    return ESP_OK;
}

static esp_err_t setup_panel_io(esp_lcd_panel_io_handle_t *panel_io) {
    esp_err_t ret = ESP_OK;

    // Konfiguracja IO dla trybu QSPI
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = GPIO_NUM_9,
        .dc_gpio_num = -1,  // DC nie jest używany w QSPI
        .spi_mode = 3,
        .pclk_hz = 40 * 1000 * 1000,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = 32,
        .lcd_param_bits = 8,
        .flags = {
            .dc_as_cmd_phase = 0,
            .dc_low_on_data = 0,
            .octal_mode = 0,
            .lsb_first = 0,
        },
    };

    ret = esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)SPI3_HOST,
        &io_config,
        panel_io
    );

    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to create panel IO: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
        return ret;
    }

    if (system_state.usb_cdc_ready) {
        Serial.printf("[%s] Panel IO (QSPI) initialized successfully\n", TAG_LCD);
    }
    return ESP_OK;
}

static esp_err_t setup_axs15231b_driver(esp_lcd_panel_io_handle_t panel_io) {
    esp_err_t ret = ESP_OK;

    // Komendy inicjalizacyjne dla AXS15231B
    static const uint8_t init_data_0[] = {0x00};
    static const uint8_t init_data_1[] = {0x00};
    
    static const axs15231b_lcd_init_cmd_t panel_init_cmds[] = {
        {0x11, (const void *)init_data_0, sizeof(init_data_0), 100},  // Sleep Out
        {0x29, (const void *)init_data_1, sizeof(init_data_1), 100},  // Display On
    };

    // Konfiguracja vendora
    axs15231b_vendor_config_t vendor_config = {
        .init_cmds = panel_init_cmds,
        .init_cmds_size = sizeof(panel_init_cmds) / sizeof(panel_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        }
    };

    // Konfiguracja panelu
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,  // Reset obsługujemy ręcznie
        .bits_per_pixel = 16,
        .flags = {
            .reset_active_high = true,
        },
        .vendor_config = (void *)&vendor_config,
    };

    ret = esp_lcd_new_panel_axs15231b(panel_io, &panel_config, &panel_handle);
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to create AXS15231B panel: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
        return ret;
    }

    // Inicjalizacja panelu
    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to initialize panel: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
        return ret;
    }

    // Disp On
    ret = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to turn on display: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
        return ret;
    }

    // ⚠️ KRYTYCZNE: Orientacja panelu (Waveshare wysyła LANDSCAPE, my chcemy PORTRAIT)
    // Waveshare 3.49" ma fizyczną rozdzielczość 640x172 (landscape)
    // Ale chcemy wyrażać to jako 172x640 (portrait dla UI)
    // Musimy powiedzieć panelowi aby zamienił osie (swap_xy)
    ret = esp_lcd_panel_swap_xy(panel_handle, true);
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to swap XY: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
    }

    // Mirror Y aby być w zgodzie z portrait mode
    ret = esp_lcd_panel_mirror(panel_handle, false, true);
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] Failed to mirror Y: %s\n", TAG_LCD, esp_err_to_name(ret));
        }
    }

    system_state.panel_initialized = true;
    if (system_state.usb_cdc_ready) {
        Serial.printf("[%s] AXS15231B driver initialized successfully (172x640 PORTRAIT)\n", TAG_LCD);
        Serial.printf("[%s] Panel XY swapped and Y mirrored for portrait orientation\n", TAG_LCD);
    }
    return ESP_OK;
}

// ============================================================================
// MEMORY SETUP - Alokacja bufora LVGL z PSRAM
// ============================================================================

static esp_err_t setup_lvgl_buffer(void) {
    esp_err_t ret = ESP_OK;

    // Alokacja z PSRAM - nie z wewnętrznego SRAM!
    buf1 = (uint16_t *)heap_caps_malloc(DRAW_BUF_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (buf1 == NULL) {
        if (system_state.usb_cdc_ready) {
            Serial.printf("[%s] FAILED to allocate %d bytes from PSRAM\n", TAG_MEM, DRAW_BUF_BYTES);
            // Fallback na SRAM (ostatnia deska ratunku, ale może być za mało)
            buf1 = (uint16_t *)heap_caps_malloc(DRAW_BUF_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
            if (buf1 == NULL) {
                Serial.printf("[%s] FATAL: Cannot allocate buffer from SRAM either!\n", TAG_MEM);
                return ESP_ERR_NO_MEM;
            }
            Serial.printf("[%s] WARNING: Using internal SRAM buffer (may be unstable)\n", TAG_MEM);
        }
        return ESP_ERR_NO_MEM;
    }

    if (system_state.usb_cdc_ready) {
        size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        Serial.printf("[%s] LVGL buffer allocated: %d bytes from PSRAM\n", TAG_MEM, DRAW_BUF_BYTES);
        Serial.printf("[%s] PSRAM status: %d/%d bytes free\n", TAG_MEM, psram_free, psram_total);
    }

    return ESP_OK;
}

// ============================================================================
// LVGL SETUP - Inicjalizacja LVGL v9
// ============================================================================

static esp_err_t setup_lvgl(void) {
    esp_err_t ret = ESP_OK;

    // Inicjalizacja jądra LVGL
    lv_init();

    // Alokacja bufora
    ret = setup_lvgl_buffer();
    if (ret != ESP_OK) {
        if (system_state.usb_cdc_ready) {
            Serial.println("[ERROR] Failed to allocate LVGL buffer!");
        }
        return ret;
    }

    // Utworzenie display obiektu
    lv_disp = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (lv_disp == NULL) {
        if (system_state.usb_cdc_ready) {
            Serial.println("[ERROR] Failed to create LVGL display object!");
        }
        return ESP_ERR_NO_MEM;
    }

    // Ustawienie bufora (rolling buffer mode)
    lv_display_set_buffers(
        lv_disp,
        buf1,
        NULL,
        DRAW_BUF_BYTES,
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    // Rejestracja callback flush
    lv_display_set_flush_cb(lv_disp, disp_flush_cb);

    // Zapisanie panel_handle w display userData dla dostępu w callback
    lv_display_set_user_data(lv_disp, panel_handle);

    paint_black_screen();

    system_state.lvgl_initialized = true;
    if (system_state.usb_cdc_ready) {
        Serial.printf("[%s] LVGL v9 initialized with %d-byte rolling buffer\n", TAG_MAIN, DRAW_BUF_BYTES);
    }

    return ESP_OK;
}

static esp_err_t setup_touch_input(void)
{
    system_i2c_init();
    system_touch_init();

    lv_touch_indev = lv_indev_create();
    if (lv_touch_indev == nullptr) {
        if (system_state.usb_cdc_ready) {
            Serial.println("[ERROR] Failed to create LVGL touch indev");
        }
        return ESP_ERR_NO_MEM;
    }

    lv_indev_set_type(lv_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lv_touch_indev, touch_indev_read_cb);
    lv_indev_set_display(lv_touch_indev, lv_disp);

    if (system_state.usb_cdc_ready) {
        Serial.printf("[%s] Touch indev registered (%d ms poll, EMA smooth)\n",
                      TAG_MAIN, ACHROM_TOUCH_POLL_MS);
    }
    return ESP_OK;
}

// ============================================================================
// LAUNCHER SETUP - Inicjalizacja UI Launchera
// ============================================================================

static esp_err_t setup_launcher(void) {
    esp_err_t ret = ESP_OK;

    ui_launcher_init(on_app_selected);

    system_state.launcher_ready = true;
    if (system_state.usb_cdc_ready) {
        Serial.println("[LAUNCHER] UI Launcher initialized and displayed");
    }

    return ESP_OK;
}

// ============================================================================
// SETUP() - Główna inicjalizacja systemu
// ============================================================================

void setup() {
    // Inicjalizacja Serial (nieblokująca, opóźniamy logi)
    Serial.begin(115200);
    delay(100);

    // Oczekiwanie na stabilizację USB CDC
    unsigned long start_time = millis();
    while (millis() - start_time < STARTUP_LOG_DELAY_MS) {
        delay(10);
    }
    system_state.usb_cdc_ready = true;

    Serial.println("\n========================================");
    Serial.println("[ ACHROMATOSIS OS SYSTEM STARTUP ]");
    Serial.println("========================================\n");

    Serial.printf("[%s] ESP32-S3 Core 0, FreeRTOS started\n", TAG_MAIN);
    Serial.printf("[%s] Building for Waveshare 3.49\" LCD (172x640, AXS15231B on QSPI)\n", TAG_MAIN);

    // ========== ETAP 1: Hardware ==========
    Serial.printf("[%s] STAGE 1/4: Hardware Initialization\n", TAG_MAIN);

    if (setup_backlight_and_reset() != ESP_OK) {
        Serial.printf("[%s] ERROR during backlight/reset setup\n", TAG_MAIN);
        return;
    }

    if (setup_spi_bus() != ESP_OK) {
        Serial.printf("[%s] ERROR during SPI bus setup\n", TAG_MAIN);
        return;
    }

    esp_lcd_panel_io_handle_t panel_io = NULL;
    if (setup_panel_io(&panel_io) != ESP_OK) {
        Serial.printf("[%s] ERROR during panel IO setup\n", TAG_MAIN);
        return;
    }

    if (setup_axs15231b_driver(panel_io) != ESP_OK) {
        Serial.printf("[%s] ERROR during AXS15231B driver setup\n", TAG_MAIN);
        return;
    }

    Serial.printf("[%s] ✓ Hardware stage complete\n\n", TAG_MAIN);

    // ========== ETAP 2: Memory & LVGL ==========
    Serial.printf("[%s] STAGE 2/4: Memory & LVGL Initialization\n", TAG_MAIN);

    if (setup_lvgl() != ESP_OK) {
        Serial.printf("[%s] ERROR during LVGL setup\n", TAG_MAIN);
        return;
    }

    if (setup_touch_input() != ESP_OK) {
        Serial.printf("[%s] ERROR during touch input setup\n", TAG_MAIN);
        return;
    }

    Serial.printf("[%s] ✓ Memory & LVGL stage complete\n\n", TAG_MAIN);

    // ========== ETAP 3: UI ==========
    Serial.printf("[%s] STAGE 3/4: UI Launcher Initialization\n", TAG_MAIN);

    if (setup_launcher() != ESP_OK) {
        Serial.printf("[%s] ERROR during launcher setup\n", TAG_MAIN);
        return;
    }

    Serial.printf("[%s] ✓ UI stage complete\n\n", TAG_MAIN);

    s_lv_tick_last_ms = millis();
    s_ui_test_t0_ms = millis();
    s_ui_test_armed = true;

    // ========== SYSTEM READY ==========
    Serial.printf("[%s] ========================================\n", TAG_MAIN);
    Serial.printf("[%s] ✓ ACHROMATOSIS OS READY\n", TAG_MAIN);
    Serial.printf("[%s] System started successfully\n", TAG_MAIN);
    Serial.printf("[%s] ========================================\n", TAG_MAIN);
}

// ============================================================================
// LOOP() - Główna pętla systemu
// ============================================================================

void loop() {
    const uint32_t now_ms = millis();

    lvgl_tick_update();
    system_touch_poll();
    app_player_poll();

    run_startup_ui_test(now_ms);

    const uint32_t time_till_next = lv_timer_handler();

    static unsigned long last_ui_tick = 0;
    if (now_ms - last_ui_tick >= LVGL_TICK_PERIOD_MS) {
        ui_launcher_tick();
        last_ui_tick = now_ms;
    }

    uint32_t sleep_ms = (time_till_next > 0) ? time_till_next : 1;
    if (sleep_ms > 10) {
        sleep_ms = 10;
    }
    delay(sleep_ms);
}