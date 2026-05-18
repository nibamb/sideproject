#include "lvgl/lvgl.h"
#include "achromatosis_config.h"
#include "ui_launcher.h"
#include <unistd.h>
#include <stdio.h>

static achromatosis_app_t current_state = APP_HUB;
static lv_obj_t *active_app_container = NULL;

static lv_timer_t *rsvp_timer = NULL;
static lv_obj_t *rsvp_word_label = NULL;
static uint32_t rsvp_wpm = 350;
static uint32_t rsvp_word_index = 0;

static lv_timer_t *gyro_timer = NULL;
static lv_obj_t *sb_sh_dark = NULL;
static lv_obj_t *sb_sh_light = NULL;
static lv_obj_t *sandbox_param_label = NULL;
static lv_obj_t *sandbox_toggle_btn = NULL;
static bool sandbox_mode_state = false;
static int32_t sandbox_param_val = 0;

static const char *rsvp_text_bank[] = {
    "SYSTEM", "ACHROMATOSIS", "URUCHOMIONY", "POZYTYWNIE.", 
    "PROCES", "SZYBKIEGO", "CZYTANIA", "RSVP", "DZIALA", 
    "W", "TRYBIE", "NATIVE", "SIMULATION", "NA", "MAC", 
    "M4.", "PREDKOSC", "MOZNA", "REGULOWAC", "SLIDEREM", 
    "U", "DOLU", "EKRANU.", "ZACHOWANIE", "PAMIECI", 
    "JEST", "W", "PELNI", "ZABEZPIECZONE", "PRZEZ", 
    "JAWNE", "CZYSZCZENIE", "STUBOW", "ORAZ", "TIMEROW."
};
static const uint32_t rsvp_text_bank_size = 35;

void app_switch_to(achromatosis_app_t new_state);
static void back_to_menu_event_cb(lv_event_t *e);
static void rsvp_timer_cb(lv_timer_t *timer);
static void rsvp_play_pause_cb(lv_event_t *e);
static void rsvp_slider_cb(lv_event_t *e);
static void sandbox_slider_cb(lv_event_t *e);
static void sandbox_toggle_event_cb(lv_event_t *e);
static void gyro_timer_cb(lv_timer_t *timer);

static void rsvp_timer_cb(lv_timer_t *timer) {
    if (!rsvp_word_label) return;
    lv_label_set_text(rsvp_word_label, rsvp_text_bank[rsvp_word_index]);
    rsvp_word_index = (rsvp_word_index + 1) % rsvp_text_bank_size;
}

static void rsvp_slider_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    rsvp_wpm = lv_slider_get_value(slider);
    if (rsvp_timer) {
        uint32_t new_period = 60000 / rsvp_wpm;
        lv_timer_set_period(rsvp_timer, new_period);
    }
}

static void rsvp_play_pause_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED && rsvp_timer) {
        if (lv_timer_get_paused(rsvp_timer)) {
            lv_timer_resume(rsvp_timer);
        } else {
            lv_timer_pause(rsvp_timer);
        }
    }
}

static void sandbox_slider_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    sandbox_param_val = lv_slider_get_value(slider);
    if (sandbox_param_label) {
        static char buf[32];
        lv_snprintf(buf, sizeof(buf), "PARAM: %d%%", sandbox_param_val);
        lv_label_set_text(sandbox_param_label, buf);
    }
}

static void sandbox_toggle_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED) {
        sandbox_mode_state = !sandbox_mode_state;
        if (sandbox_toggle_btn) {
            lv_obj_t *lbl = lv_obj_get_child(sandbox_toggle_btn, 0);
            if (lbl) {
                lv_label_set_text((lv_obj_t *)lbl, sandbox_mode_state ? "MODE: ACTIVE" : "MODE: IDLE");
            }
        }
    }
}

static void back_to_menu_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_RELEASED) {
        app_switch_to(APP_HUB);
    }
}

static void gyro_timer_cb(lv_timer_t *timer) {
    lv_indev_t *indev = lv_indev_get_next(NULL);
    if (!indev) return;
    lv_point_t mouse_pos;
    lv_indev_get_point(indev, &mouse_pos);
    int diff_x = (mouse_pos.x - 320) / 40; 
    int diff_y = (mouse_pos.y - 86) / 15;
    if(diff_x > 6) diff_x = 6; if(diff_x < -6) diff_x = -6;
    if(diff_y > 4) diff_y = 4; if(diff_y < -4) diff_y = -4;
    if (sb_sh_dark && sb_sh_light) {
        lv_obj_set_style_translate_x(sb_sh_dark, diff_x, LV_PART_MAIN);
        lv_obj_set_style_translate_y(sb_sh_dark, diff_y, LV_PART_MAIN);
        lv_obj_set_style_translate_x(sb_sh_light, -diff_x, LV_PART_MAIN);
        lv_obj_set_style_translate_y(sb_sh_light, -diff_y, LV_PART_MAIN);
    }
}

void app_switch_to(achromatosis_app_t new_state) {
    if (current_state == APP_HUB) {
        ui_launcher_hide();
    } else {
        if (rsvp_timer) {
            lv_timer_delete(rsvp_timer);
            rsvp_timer = NULL;
        }
        if (gyro_timer) {
            lv_timer_delete(gyro_timer);
            gyro_timer = NULL;
        }
        if (active_app_container) {
            lv_obj_delete(active_app_container);
            active_app_container = NULL;
        }
        sb_sh_dark = NULL;
        sb_sh_light = NULL;
        sandbox_param_label = NULL;
        sandbox_toggle_btn = NULL;
        sandbox_mode_state = false;
        sandbox_param_val = 0;
    }

    current_state = new_state;

    if (new_state == APP_HUB) {
        ui_launcher_show();
        return;
    }

    active_app_container = lv_obj_create(lv_screen_active());
    // Horyzontalny widok aplikacji: Szerokość 640, Wysokość 172
    lv_obj_set_size(active_app_container, 640, 172);
    lv_obj_align(active_app_container, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(active_app_container, lv_color_hex(0xDCDCDC), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(active_app_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(active_app_container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(active_app_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(active_app_container, 0, LV_PART_MAIN);
    lv_obj_remove_flag(active_app_container, LV_OBJ_FLAG_SCROLLABLE);

    if (new_state == APP_RSVP_READER) {
        lv_obj_t *title = lv_label_create(active_app_container);
        lv_label_set_text(title, "RSVP READER");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(title, lv_color_hex(0x282828), LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 15);

        rsvp_word_label = lv_label_create(active_app_container);
        lv_label_set_text(rsvp_word_label, "READY");
        lv_obj_set_style_text_font(rsvp_word_label, &lv_font_montserrat_18, LV_PART_MAIN);
        lv_obj_set_style_text_color(rsvp_word_label, lv_color_hex(0x282828), LV_PART_MAIN);
        lv_obj_align(rsvp_word_label, LV_ALIGN_CENTER, -60, -10);
        rsvp_word_index = 0;

        uint32_t initial_period = 60000 / rsvp_wpm;
        rsvp_timer = lv_timer_create(rsvp_timer_cb, initial_period, NULL);
        lv_timer_pause(rsvp_timer);

        lv_obj_t *btn_play = lv_button_create(active_app_container);
        lv_obj_set_size(btn_play, 130, 40);
        lv_obj_align(btn_play, LV_ALIGN_CENTER, 140, -10);
        lv_obj_set_style_bg_color(btn_play, lv_color_hex(0xD4D4D4), LV_PART_MAIN);
        lv_obj_set_style_radius(btn_play, 10, LV_PART_MAIN);
        lv_obj_add_event_cb(btn_play, rsvp_play_pause_cb, LV_EVENT_RELEASED, NULL);

        lv_obj_t *lbl_play = lv_label_create(btn_play);
        lv_label_set_text(lbl_play, "PLAY / PAUSE");
        lv_obj_set_style_text_font(lbl_play, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_play, lv_color_hex(0x282828), LV_PART_MAIN);
        lv_obj_align(lbl_play, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t *slider = lv_slider_create(active_app_container);
        lv_obj_set_size(slider, 200, 10);
        lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, -60, -20);
        lv_slider_set_range(slider, 200, 1000);
        lv_slider_set_value(slider, rsvp_wpm, LV_ANIM_OFF);
        lv_obj_add_event_cb(slider, rsvp_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    } else if (new_state == APP_SANDBOX) {
        sandbox_param_label = lv_label_create(active_app_container);
        char buf[32];
        snprintf(buf, sizeof(buf), "PARAM: %d%%", sandbox_param_val);
        lv_label_set_text(sandbox_param_label, buf);
        lv_obj_set_style_text_font(sandbox_param_label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(sandbox_param_label, lv_color_hex(0x282828), LV_PART_MAIN);
        lv_obj_align(sandbox_param_label, LV_ALIGN_TOP_LEFT, 20, 50);

        lv_obj_t *slider = lv_slider_create(active_app_container);
        lv_obj_set_size(slider, 180, 10);
        lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 20, 85);
        lv_slider_set_range(slider, 0, 100);
        lv_slider_set_value(slider, sandbox_param_val, LV_ANIM_OFF);
        lv_obj_add_event_cb(slider, sandbox_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

        sb_sh_dark = lv_obj_create(active_app_container);
        lv_obj_set_size(sb_sh_dark, 140, 45);
        lv_obj_align(sb_sh_dark, LV_ALIGN_CENTER, 80, -10);
        lv_obj_set_style_bg_color(sb_sh_dark, lv_color_hex(0xB8B8B8), LV_PART_MAIN);
        lv_obj_set_style_border_width(sb_sh_dark, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(sb_sh_dark, 14, LV_PART_MAIN);
        lv_obj_remove_flag(sb_sh_dark, LV_OBJ_FLAG_CLICKABLE);

        sb_sh_light = lv_obj_create(active_app_container);
        lv_obj_set_size(sb_sh_light, 140, 45);
        lv_obj_align(sb_sh_light, LV_ALIGN_CENTER, 80, -10);
        lv_obj_set_style_bg_color(sb_sh_light, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_border_width(sb_sh_light, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(sb_sh_light, 14, LV_PART_MAIN);
        lv_obj_remove_flag(sb_sh_light, LV_OBJ_FLAG_CLICKABLE);

        sandbox_toggle_btn = lv_button_create(active_app_container);
        lv_obj_set_size(sandbox_toggle_btn, 140, 45);
        lv_obj_align(sandbox_toggle_btn, LV_ALIGN_CENTER, 80, -10);
        lv_obj_set_style_bg_color(sandbox_toggle_btn, lv_color_hex(0xD8D8D8), LV_PART_MAIN);
        lv_obj_set_style_radius(sandbox_toggle_btn, 14, LV_PART_MAIN);
        lv_obj_add_event_cb(sandbox_toggle_btn, sandbox_toggle_event_cb, LV_EVENT_RELEASED, NULL);

        lv_obj_t *lbl_toggle = lv_label_create(sandbox_toggle_btn);
        lv_label_set_text(lbl_toggle, sandbox_mode_state ? "MODE: ACTIVE" : "MODE: IDLE");
        lv_obj_set_style_text_font(lbl_toggle, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(lbl_toggle, lv_color_hex(0x282828), LV_PART_MAIN);
        lv_obj_align(lbl_toggle, LV_ALIGN_CENTER, 0, 0);

        gyro_timer = lv_timer_create(gyro_timer_cb, 33, NULL);

    } else {
        lv_obj_t *title = lv_label_create(active_app_container);
        const char *mockup_title = (new_state == APP_PLAYER) ? "PLAYER" :
                                   (new_state == APP_OPTIONS) ? "OPTIONS" : "UNKNOWN";
        lv_label_set_text(title, mockup_title);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(title, lv_color_hex(0x282828), LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

        lv_obj_t *stub_lbl = lv_label_create(active_app_container);
        lv_label_set_text(stub_lbl, "MOCKUP VIEW");
        lv_obj_set_style_text_font(stub_lbl, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(stub_lbl, lv_color_hex(0xB8B8B8), LV_PART_MAIN);
        lv_obj_align(stub_lbl, LV_ALIGN_CENTER, 0, 0);
    }

    lv_obj_t *btn_back = lv_button_create(active_app_container);
    lv_obj_set_size(btn_back, 130, 40);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0xC0C0C0), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_back, 18, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_back, back_to_menu_event_cb, LV_EVENT_RELEASED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "BACK");
    lv_obj_set_style_text_font(lbl_back, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_back, lv_color_hex(0x282828), LV_PART_MAIN);
    lv_obj_align(lbl_back, LV_ALIGN_CENTER, 0, 0);
}

static void on_launcher_app_selected(achromatosis_app_t app) {
    app_switch_to(app);
}

int main(int argc, char **argv) {
    lv_init();
    // Zmiana rozmiaru okna SDL: Szerokość 640, Wysokość 172
    lv_sdl_window_create(640, 172);

    ui_launcher_init(on_launcher_app_selected);

    while (1) {
        uint32_t time_till_next = lv_timer_handler();
        usleep(time_till_next * 1000);
    }

    return 0;
}