#include <cstdint>
#include "ui_launcher.h"
#include "achromatosis_config.h"
#include "lvgl/lvgl.h"

static lv_style_t style_neumorphic_frame;
static lv_style_t style_neumorphic_pill_btn;
static lv_style_t style_neumorphic_pill_btn_pr;
static lv_style_t style_neumorphic_shadow_dark;
static lv_style_t style_neumorphic_shadow_light;
static lv_style_t style_text_main;

static bool s_styles_inited = false;

typedef struct {
    lv_obj_t *shadow_dark;
    lv_obj_t *shadow_light;
} launcher_neum_shadows_t;

static struct {
    lv_obj_t *root;
    bool built;
} launcher_state = { nullptr, false };

static ui_launcher_state_t s_pub_state = {};
static launcher_neum_shadows_t s_row_shadows[4];
static lv_obj_t *s_row_pills[4] = {nullptr, nullptr, nullptr, nullptr};
static lv_obj_t *s_row_labels[4] = {nullptr, nullptr, nullptr, nullptr};

static const char* s_app_names[4] = {
    "CHROMA MIDI",
    "CHROMA RSVP",
    "CHROMA SAND",
    "DRIFTONE"
};

static void init_styles(void)
{
    if (s_styles_inited) return;

    // Tło ramki zewnętrznej
    lv_style_init(&style_neumorphic_frame);
    lv_style_set_bg_color(&style_neumorphic_frame, lv_color_hex(ACHROM_COLOR_BG));
    lv_style_set_radius(&style_neumorphic_frame, ACHROM_NEUM_FRAME_RADIUS);
    lv_style_set_border_width(&style_neumorphic_frame, 0);

    // Ciemny cień bazowy zewnętrzny
    lv_style_init(&style_neumorphic_shadow_dark);
    lv_style_set_bg_color(&style_neumorphic_shadow_dark, lv_color_hex(ACHROM_COLOR_SHADOW_DARK));
    lv_style_set_radius(&style_neumorphic_shadow_dark, ACHROM_NEUM_PILL_RADIUS);

    // Jasny cień bazowy zewnętrzny
    lv_style_init(&style_neumorphic_shadow_light);
    lv_style_set_bg_color(&style_neumorphic_shadow_light, lv_color_hex(ACHROM_COLOR_SHADOW_LIGHT));
    lv_style_set_radius(&style_neumorphic_shadow_light, ACHROM_NEUM_PILL_RADIUS);

    // Przycisk wypukły (stan domyślny)
    lv_style_init(&style_neumorphic_pill_btn);
    lv_style_set_bg_color(&style_neumorphic_pill_btn, lv_color_hex(ACHROM_ROW1_PILL_TOP));
    lv_style_set_radius(&style_neumorphic_pill_btn, ACHROM_NEUM_PILL_RADIUS);
    lv_style_set_border_width(&style_neumorphic_pill_btn, 0);

    // Przycisk wciśnięty (stan aktywny)
    lv_style_init(&style_neumorphic_pill_btn_pr);
    lv_style_set_bg_color(&style_neumorphic_pill_btn_pr, lv_color_hex(ACHROM_ROW2_PILL_TOP));

    // Czcionka i tekst
    lv_style_init(&style_text_main);
    lv_style_set_text_color(&style_text_main, lv_color_hex(ACHROM_COLOR_TEXT));
    lv_style_set_text_font(&style_text_main, ACHROM_FONT_MAIN);

    s_styles_inited = true;
}

static void pill_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    intptr_t row_idx = (intptr_t)lv_event_get_user_data(e);
    launcher_neum_shadows_t *pair = &s_row_shadows[row_idx];

    if (code == LV_EVENT_PRESSED) {
        // Kliknięcie - ukrywamy cienie imitując zapadnięcie przycisku
        if (pair->shadow_dark) lv_obj_set_style_opa(pair->shadow_dark, LV_OPA_0, LV_PART_MAIN);
        if (pair->shadow_light) lv_obj_set_style_opa(pair->shadow_light, LV_OPA_0, LV_PART_MAIN);
    }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        // Puszczenie myszki - przywracamy cienie (przycisk wyskakuje)
        if (pair->shadow_dark) lv_obj_set_style_opa(pair->shadow_dark, LV_OPA_COVER, LV_PART_MAIN);
        if (pair->shadow_light) lv_obj_set_style_opa(pair->shadow_light, LV_OPA_COVER, LV_PART_MAIN);
    }
}

extern "C" void ui_launcher_create(lv_obj_t *parent)
{
    init_styles();

    if (launcher_state.built) return;

    launcher_state.root = lv_obj_create(parent);
    lv_obj_set_size(launcher_state.root, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_style_bg_color(launcher_state.root, lv_color_hex(ACHROM_COLOR_BG), LV_PART_MAIN);
    lv_obj_set_style_border_width(launcher_state.root, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(launcher_state.root, 0, LV_PART_MAIN);
    lv_obj_remove_flag(launcher_state.root, LV_OBJ_FLAG_SCROLLABLE);

    int start_y = 40;

    for (int i = 0; i < 4; i++) {
        int y_pos = start_y + i * (ACHROM_LAUNCHER_FRAME_H + ACHROM_LAUNCHER_ROW_GAP);

        // Kontener zewnętrzny przycisku
        lv_obj_t *frame = lv_obj_create(launcher_state.root);
        lv_obj_set_size(frame, ACHROM_LAUNCHER_FRAME_W, ACHROM_LAUNCHER_FRAME_H);
        lv_obj_align(frame, LV_ALIGN_TOP_MID, 0, y_pos);
        lv_obj_add_style(frame, &style_neumorphic_frame, LV_PART_MAIN);
        lv_obj_set_style_pad_all(frame, 0, LV_PART_MAIN);
        lv_obj_remove_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

        // Tworzenie ciemnego cienia pod przyciskiem
        lv_obj_t *sh_dark = lv_obj_create(frame);
        lv_obj_set_size(sh_dark, ACHROM_LAUNCHER_PILL_W, ACHROM_LAUNCHER_PILL_H);
        lv_obj_align(sh_dark, LV_ALIGN_TOP_LEFT, ACHROM_LAUNCHER_PAD_X + 4, 4 + 4);
        lv_obj_add_style(sh_dark, &style_neumorphic_shadow_dark, LV_PART_MAIN);
        lv_obj_set_style_border_width(sh_dark, 0, LV_PART_MAIN);

        // Tworzenie jasnego cienia nad przyciskiem
        lv_obj_t *sh_light = lv_obj_create(frame);
        lv_obj_set_size(sh_light, ACHROM_LAUNCHER_PILL_W, ACHROM_LAUNCHER_PILL_H);
        lv_obj_align(sh_light, LV_ALIGN_TOP_LEFT, ACHROM_LAUNCHER_PAD_X - 4, 4 - 4);
        lv_obj_add_style(sh_light, &style_neumorphic_shadow_light, LV_PART_MAIN);
        lv_obj_set_style_border_width(sh_light, 0, LV_PART_MAIN);

        s_row_shadows[i].shadow_dark = sh_dark;
        s_row_shadows[i].shadow_light = sh_light;

        // Główny element interaktywny (Pigułka)
        lv_obj_t *pill = lv_obj_create(frame);
        lv_obj_set_size(pill, ACHROM_LAUNCHER_PILL_W, ACHROM_LAUNCHER_PILL_H);
        lv_obj_align(pill, LV_ALIGN_TOP_LEFT, ACHROM_LAUNCHER_PAD_X, 4);
        lv_obj_add_style(pill, &style_neumorphic_pill_btn, LV_PART_MAIN);
        lv_obj_add_style(pill, &style_neumorphic_pill_btn_pr, LV_STATE_PRESSED);
        lv_obj_set_style_pad_all(pill, 0, LV_PART_MAIN);
        lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE);

        s_row_pills[i] = pill;

        // Etykieta tekstowa na środku pigułki
        lv_obj_t *label = lv_label_create(pill);
        lv_label_set_text(label, s_app_names[i]);
        lv_obj_add_style(label, &style_text_main, LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        s_row_labels[i] = label;

        // Przypięcie obsługi myszki dla animacji neumorficznej
        lv_obj_add_event_cb(pill, pill_event_cb, LV_EVENT_ALL, (void*)(intptr_t)i);
    }

    launcher_state.built = true;
    s_pub_state.is_visible = true;
}

extern "C" void ui_launcher_show(void)
{
    if (launcher_state.root != nullptr) {
        lv_obj_remove_flag(launcher_state.root, LV_OBJ_FLAG_HIDDEN);
    }
    s_pub_state.is_visible = true;
}

extern "C" void ui_launcher_hide(void)
{
    if (launcher_state.root != nullptr) {
        lv_obj_add_flag(launcher_state.root, LV_OBJ_FLAG_HIDDEN);
    }
    s_pub_state.is_visible = false;
}