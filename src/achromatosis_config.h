#ifndef ACHROMATOSIS_CONFIG_H
#define ACHROMATOSIS_CONFIG_H

#include <lvgl.h>

// =============================================================================
// STRUKTURA SYSTEMU APLIKACJI (Maszyna Stanów)
// =============================================================================
typedef enum {
    APP_HUB,
    APP_PLAYER,
    APP_RSVP_READER,
    APP_SANDBOX,
    APP_CHROMA_CONTROL,
    APP_OPTIONS
} achromatosis_app_t;

// =============================================================================
// KONFIGURACJA EKRANU (Wymiary Waveshare 3.49" Touch LCD)
// =============================================================================
#define DISPLAY_WIDTH             172
#define DISPLAY_HEIGHT            640

// =============================================================================
// MAPOWANIE PINÓW MAGISTRALI QSPI (Zgodnie z Waveshare)
// =============================================================================
#define LCD_HOST                  SPI3_HOST      // Kluczowe: SPI3 zamiast SPI2!
#define LCD_CS                    GPIO_NUM_9
#define LCD_PCLK                  GPIO_NUM_10
#define LCD_DATA0                 GPIO_NUM_11
#define LCD_DATA1                 GPIO_NUM_12
#define LCD_DATA2                 GPIO_NUM_13
#define LCD_DATA3                 GPIO_NUM_14
#define LCD_RST                   GPIO_NUM_21
#define LCD_BL                    GPIO_NUM_8        // Oficjalny pin podświetlenia!

#define DISPLAY_POWER_ENABLE      ((gpio_num_t)-1)

// =============================================================================
// KONFIGURACJA BUFORÓW I TAKTOWANIA LVGL
// =============================================================================
#define LVGL_TICK_PERIOD_MS       5
#define LVGL_DMA_BUFF_LEN         (DISPLAY_WIDTH * 64 * 2)

// =============================================================================
// MAGISTRALA I2C (Dotyk, Akcelerometr/IMU)
// =============================================================================
#define TOUCH_SCL                 GPIO_NUM_18
#define TOUCH_SDA                 GPIO_NUM_17

#define GT911_I2C_ADDR            0x14  // Standardowy adres dotyku GT911 (bywa też 0x5D)
#define DISP_TOUCH_ADDR           0x3B  // Alternatywny adres kontrolera z user_config
#define IMU_ADDR                  0x6B  // Standardowy adres dla QMI8658 na płytkach Waveshare

// =============================================================================
// KARTA SD (Tryb 1-bit / 4-bit SD_MMC dla ESP32-S3 Waveshare)
// =============================================================================
#define SD_MMC_CLK                GPIO_NUM_39
#define SD_MMC_CMD                GPIO_NUM_38
#define SD_MMC_D0                 GPIO_NUM_40
#define SD_MMC_D1                 GPIO_NUM_41
#define SD_MMC_D2                 GPIO_NUM_42
#define SD_MMC_D3                 GPIO_NUM_44

// =============================================================================
// PALETA — NEUMORFIZM (achromatyczny, z pionowym „zatopieniem” wierszy)
// =============================================================================
// --- Bazowa paleta (zgodnie ze specyfikacją wizualną) ---
#define ACHROM_COLOR_RECESS_BASE        0xC0C0C0
#define ACHROM_COLOR_PILL_SURFACE       0xD0D0D0
#define ACHROM_COLOR_DARK_SHADOW        0x202020
#define ACHROM_COLOR_HIGHLIGHT_SHADOW   0xF0F0F0
#define ACHROM_COLOR_TEXT               0x101010

// --- Tło ekranu Launchera (głębokie, neutralne; kontrast dla pigułek) ---
#define ACHROM_COLOR_SCREEN_GRAD_TOP    0x1A1A1A
#define ACHROM_COLOR_SCREEN_GRAD_BOT    0x080808

// --- Ramka wgłębiona (gradient: góra ciemniejsza → dół jaśniejszy): inset ---
#define ACHROM_COLOR_FRAME_INSET_TOP    0xA8A8A8
#define ACHROM_COLOR_FRAME_INSET_BOT    0xD8D8D8

// --- Delikatny „wewnętrzny” cień ramki (górna krawędź) ---
#define ACHROM_COLOR_FRAME_INNER_SHADOW 0x606060

// --- Pigułka: gradient powierzchni (światło z góry-lewej → dół-prawy, VER jako przybliżenie) ---
#define ACHROM_COLOR_PILL_GRAD_TOP      0xE8E8E8
#define ACHROM_COLOR_PILL_GRAD_BOT      0xC8C8C8

// --- Pigułka w stanie PRESSED (spłaszczenie, mniej „lift”) ---
#define ACHROM_COLOR_PILL_PRESSED_TOP   0xC8C8C8
#define ACHROM_COLOR_PILL_PRESSED_BOT   0xB0B0B0

// --- Per-wiersz: globalny gradient kolumny (Magnesium jaśniej → Jupiter ciemniej) ---
#define ACHROM_ROW0_RECESS_TOP          0xB0B0B0
#define ACHROM_ROW0_RECESS_BOT          0xD4D4D4
#define ACHROM_ROW1_RECESS_TOP          0xA8A8A8
#define ACHROM_ROW1_RECESS_BOT          0xCCCCCC
#define ACHROM_ROW2_RECESS_TOP          0x989898
#define ACHROM_ROW2_RECESS_BOT          0xB8B8B8
#define ACHROM_ROW3_RECESS_TOP          0x888888
#define ACHROM_ROW3_RECESS_BOT          0xA8A8A8

#define ACHROM_ROW0_PILL_TOP            0xECECEC
#define ACHROM_ROW0_PILL_BOT            0xD4D4D4
#define ACHROM_ROW1_PILL_TOP            0xE4E4E4
#define ACHROM_ROW1_PILL_BOT            0xC8C8C8
#define ACHROM_ROW2_PILL_TOP            0xD8D8D8
#define ACHROM_ROW2_PILL_BOT            0xB8B8B8
#define ACHROM_ROW3_PILL_TOP            0xC8C8C8
#define ACHROM_ROW3_PILL_BOT            0xA8A8A8

// --- FONT ---
#define ACHROM_FONT_MAIN                (&lv_font_montserrat_24)

// --- Geometria Launchera (portrait 172×640) ---
#define ACHROM_LAUNCHER_FRAME_W         160
#define ACHROM_LAUNCHER_FRAME_H         70
#define ACHROM_LAUNCHER_PILL_W          140
#define ACHROM_LAUNCHER_PILL_H          55
#define ACHROM_LAUNCHER_PAD_X           6
#define ACHROM_LAUNCHER_ROW_GAP         12
#define ACHROM_NEUM_PILL_RADIUS         18
#define ACHROM_NEUM_FRAME_RADIUS        10
#define ACHROM_NEUM_SHADOW_OFS          8
#define ACHROM_NEUM_SHADOW_WIDTH        18
#define ACHROM_NEUM_FRAME_INSET_SHADOW_W   28
#define ACHROM_NEUM_FRAME_INSET_SHADOW_OFS_Y (-6)

// --- Neumorficzne warstwy cienia (LVGL: jeden cień / obiekt — drugi cień = osobny lv_obj) ---
#define ACHROM_NEUM_SHADOW_DARK_OPA     LV_OPA_70
#define ACHROM_NEUM_SHADOW_LIGHT_OPA    LV_OPA_80
#define ACHROM_NEUM_FRAME_INSET_SHADOW_OPA LV_OPA_30

// --- Dotyk AXS15231B (wzorowany na rsvpnano, szybszy polling) ---
#define ACHROM_TOUCH_I2C_ADDR           DISP_TOUCH_ADDR
#define ACHROM_TOUCH_POLL_MS            10
#define ACHROM_TOUCH_SMOOTH_ALPHA       40
#define ACHROM_TOUCH_RELEASE_SAMPLES    2

#endif // ACHROMATOSIS_CONFIG_H
