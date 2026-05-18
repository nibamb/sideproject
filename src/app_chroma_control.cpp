#include <Arduino.h>
#include "app_chroma_control.h"
#include "achromatosis_config.h"
#include "system_i2c.h"
#include <lvgl.h>
#include <MIDIUSB.h>

static struct {
    lv_obj_t* screen;
    lv_obj_t* quadrant_labels[8];
    lv_obj_t* waveform_label;
    lv_obj_t* back_btn;
    chroma_back_cb on_back;
    int values[8];
    int last_tilt_x;
    int last_tilt_y;
    unsigned long last_tilt_ms;
} control_state = {0};

static const uint8_t control_ccs[8] = {74, 94, 71, 72, 73, 75, 76, 77};
static const char* control_names[8] = {
    "OSC WAVE", "OSC DETUNE",
    "FILT CUT", "FILT RES",
    "ENV ATT", "ENV REL",
    "LFO RATE", "LFO DEPTH",
};

static void send_midi_cc(uint8_t cc, uint8_t value) {
    midiEventPacket_t packet = {0x0B, 0xB0, cc, value};
    MidiUSB.sendMIDI(packet);
    MidiUSB.flush();
}

static void reset_param_style(lv_timer_t* timer) {
    lv_obj_t* button = (lv_obj_t*)lv_timer_get_user_data(timer);
    if (button) {
        lv_obj_set_style_bg_color(button, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_text_color(button, lv_color_white(), LV_PART_MAIN);
    }
    lv_timer_del(timer);
}

static void param_click_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    if (!btn) {
        return;
    }

    int param_id = (int)(intptr_t)lv_event_get_user_data(e);
    if (param_id < 0 || param_id >= 8) {
        return;
    }
    control_state.values[param_id] = (control_state.values[param_id] + 16) % 128;
    send_midi_cc(control_ccs[param_id], control_state.values[param_id]);

    if (control_state.quadrant_labels[param_id]) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s %d", control_names[param_id], control_state.values[param_id]);
        lv_label_set_text(control_state.quadrant_labels[param_id], buf);
    }

    lv_obj_set_style_bg_color(btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, lv_color_black(), LV_PART_MAIN);
    lv_timer_t* timer = lv_timer_create(reset_param_style, 100, btn);
    if (timer) {
        lv_timer_set_repeat_count(timer, 1);
    }
}

static void back_click_cb(lv_event_t* e) {
    (void)e;
    if (control_state.on_back) {
        control_state.on_back();
    }
}

void app_chroma_control_init(chroma_back_cb on_back) {
    control_state.on_back = on_back;
    control_state.screen = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(control_state.screen, lv_color_black(), 0);

    lv_obj_t* title = lv_label_create(control_state.screen);
    lv_label_set_text(title, "CHROMA CONTROL");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    control_state.back_btn = lv_btn_create(control_state.screen);
    lv_obj_set_size(control_state.back_btn, 90, 30);
    lv_obj_align(control_state.back_btn, LV_ALIGN_TOP_RIGHT, -8, 10);
    lv_obj_set_style_bg_color(control_state.back_btn, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_color(control_state.back_btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_add_event_cb(control_state.back_btn, back_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* back_label = lv_label_create(control_state.back_btn);
    lv_label_set_text(back_label, "[ HUB ]");
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);

    const int quad_w = 216;
    const int quad_h = 150;
    const int margin = 8;

    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 2; col++) {
            int base = (row * 2 + col) * 2;
            lv_obj_t* container = lv_obj_create(control_state.screen);
            lv_obj_set_size(container, quad_w, quad_h);
            lv_obj_set_pos(container,
                margin + col * (quad_w + margin),
                60 + row * (quad_h + margin));
            lv_obj_set_style_bg_color(container, lv_color_black(), LV_PART_MAIN);
            lv_obj_set_style_border_color(container, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_border_width(container, 2, LV_PART_MAIN);
            lv_obj_set_style_radius(container, 0, LV_PART_MAIN);

            for (int i = 0; i < 2; i++) {
                int param_id = base + i;
                lv_obj_t* btn = lv_btn_create(container);
                lv_obj_set_size(btn, quad_w - 12, 48);
                lv_obj_set_pos(btn, 6, 10 + i * 60);
                lv_obj_set_style_bg_color(btn, lv_color_black(), LV_PART_MAIN);
                lv_obj_set_style_border_color(btn, lv_color_white(), LV_PART_MAIN);
                lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
                lv_obj_add_event_cb(btn, param_click_cb, LV_EVENT_CLICKED, (void*)(intptr_t)param_id);

                lv_obj_t* label = lv_label_create(btn);
                char buf[32];
                snprintf(buf, sizeof(buf), "%s 0", control_names[param_id]);
                lv_label_set_text(label, buf);
                lv_obj_set_style_text_color(label, lv_color_white(), 0);
                lv_obj_center(label);
                control_state.quadrant_labels[param_id] = label;
            }
        }
    }

    control_state.waveform_label = lv_label_create(control_state.screen);
    lv_label_set_text(control_state.waveform_label, "~ /\\ \\_/ ~");
    lv_obj_set_style_text_color(control_state.waveform_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(control_state.waveform_label, &lv_font_montserrat_14, 0);
    lv_obj_align(control_state.waveform_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    for (int i = 0; i < 8; i++) {
        control_state.values[i] = 0;
    }

    Serial.println("[CONTROL] CHROMA CONTROL initialized");
}

void app_chroma_control_show(void) {
    if (control_state.screen) {
        lv_obj_clear_flag(control_state.screen, LV_OBJ_FLAG_HIDDEN);
    }
}

void app_chroma_control_hide(void) {
    if (control_state.screen) {
        lv_obj_add_flag(control_state.screen, LV_OBJ_FLAG_HIDDEN);
    }
}

void app_chroma_control_tick(void) {
    unsigned long now = millis();
    if (now - control_state.last_tilt_ms < 120) {
        return;
    }
    control_state.last_tilt_ms = now;

    int tilt_x = system_i2c_read_tilt_x();
    int tilt_y = system_i2c_read_tilt_y();
    if (tilt_x != control_state.last_tilt_x) {
        control_state.last_tilt_x = tilt_x;
        uint8_t value = constrain((tilt_x + 128) / 4, 0, 127);
        send_midi_cc(81, value);
    }
    if (tilt_y != control_state.last_tilt_y) {
        control_state.last_tilt_y = tilt_y;
        uint8_t value = constrain((tilt_y + 128) / 4, 0, 127);
        send_midi_cc(82, value);
    }
}
