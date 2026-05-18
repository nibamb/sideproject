#include "achromatosis_config.h"
#include "ui_launcher.h"

static lv_obj_t *main_menu_container = NULL;
static launcher_app_select_cb app_callback = NULL;

typedef struct {
    const char *title;
    achromatosis_app_t app_id;
    lv_color_t color_top;
    lv_color_t color_bot;
} launcher_item_t;

static const launcher_item_t menu_items[4] = {
    {"PLAYER",       APP_PLAYER,      {0xD4, 0xD4, 0xD4}, {0xD4, 0xD4, 0xD4}},
    {"RSVP READER",  APP_RSVP_READER, {0xE4, 0xE4, 0xE4}, {0xC8, 0xC8, 0xC8}},
    {"SANDBOX",      APP_SANDBOX,     {0xD8, 0xD8, 0xD8}, {0xB8, 0xB8, 0xB8}},
    {"OPTIONS",      APP_OPTIONS,     {0xC8, 0xC8, 0xC8}, {0xA8, 0xA8, 0xA8}}
};

typedef struct {
    achromatosis_app_t app_id;
    lv_obj_t *shadow_dark;
    lv_obj_t *shadow_light;
} pill_user_data_t;

static pill_user_data_t dynamic_pills_data[4];

static void pill_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    pill_user_data_t *data = (pill_user_data_t *)lv_event_get_user_data(e);
    
    if(!data) return;

    if(code == LV_EVENT_PRESSED) {
        if(data->shadow_dark)  lv_obj_set_style_opa(data->shadow_dark, LV_OPA_0, LV_PART_MAIN);
        if(data->shadow_light) lv_obj_set_style_opa(data->shadow_light, LV_OPA_0, LV_PART_MAIN);
    }
    else if(code == LV_EVENT_RELEASED) {
        if(data->shadow_dark)  lv_obj_set_style_opa(data->shadow_dark, LV_OPA_COVER, LV_PART_MAIN);
        if(data->shadow_light) lv_obj_set_style_opa(data->shadow_light, LV_OPA_COVER, LV_PART_MAIN);
        if(app_callback) {
            app_callback(data->app_id);
        }
    }
    else if(code == LV_EVENT_PRESS_LOST) {
        if(data->shadow_dark)  lv_obj_set_style_opa(data->shadow_dark, LV_OPA_COVER, LV_PART_MAIN);
        if(data->shadow_light) lv_obj_set_style_opa(data->shadow_light, LV_OPA_COVER, LV_PART_MAIN);
    }
}

void ui_launcher_register_callback(launcher_app_select_cb cb) {
    app_callback = cb;
}

void ui_launcher_init(launcher_app_select_cb callback) {
    app_callback = callback;

    main_menu_container = lv_obj_create(lv_screen_active());
    // Zmiana orientacji ekranu: Szerokość 640, Wysokość 172
    lv_obj_set_size(main_menu_container, 640, 172);
    lv_obj_align(main_menu_container, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_set_style_bg_color(main_menu_container, lv_color_hex(0xDCDCDC), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(main_menu_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_menu_container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(main_menu_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(main_menu_container, 0, LV_PART_MAIN);
    
    // Rozmieszczamy kafelki w poziomie (oś X)
    int current_x = 24; 
    int current_y = 58; // Wycentrowanie w pionie dla wysokości 172
    
    for(int i = 0; i < 4; i++) {
        lv_obj_t *sh_dark = lv_obj_create(main_menu_container);
        lv_obj_set_size(sh_dark, 130, 55);
        lv_obj_align(sh_dark, LV_ALIGN_TOP_LEFT, current_x + 6, current_y + 6);
        lv_obj_set_style_bg_color(sh_dark, lv_color_hex(0xB8B8B8), LV_PART_MAIN);
        lv_obj_set_style_border_width(sh_dark, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(sh_dark, 18, LV_PART_MAIN);
        
        lv_obj_t *sh_light = lv_obj_create(main_menu_container);
        lv_obj_set_size(sh_light, 130, 55);
        lv_obj_align(sh_light, LV_ALIGN_TOP_LEFT, current_x - 3, current_y - 3);
        lv_obj_set_style_bg_color(sh_light, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_border_width(sh_light, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(sh_light, 18, LV_PART_MAIN);

        lv_obj_t *pill = lv_obj_create(main_menu_container);
        lv_obj_set_size(pill, 130, 55);
        lv_obj_align(pill, LV_ALIGN_TOP_LEFT, current_x, current_y);
        lv_obj_set_style_bg_color(pill, menu_items[i].color_top, LV_PART_MAIN);
        lv_obj_set_style_border_width(pill, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(pill, 18, LV_PART_MAIN);
        lv_obj_set_style_pad_all(pill, 0, LV_PART_MAIN);
        lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

        dynamic_pills_data[i].app_id = menu_items[i].app_id;
        dynamic_pills_data[i].shadow_dark = sh_dark;
        dynamic_pills_data[i].shadow_light = sh_light;
        lv_obj_add_event_cb(pill, pill_event_cb, LV_EVENT_ALL, &dynamic_pills_data[i]);

        lv_obj_t *label = lv_label_create(pill);
        lv_label_set_text(label, menu_items[i].title);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0x282828), LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

        // Przesuwamy następny przycisk w prawo
        current_x += 130 + 24;
    }
}

void ui_launcher_show(void) {
    if(main_menu_container) {
        lv_obj_remove_flag(main_menu_container, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_launcher_hide(void) {
    if(main_menu_container) {
        lv_obj_add_flag(main_menu_container, LV_OBJ_FLAG_HIDDEN);
    }
}