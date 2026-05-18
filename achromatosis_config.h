#ifndef ACHROMATOSIS_CONFIG_H
#define ACHROMATOSIS_CONFIG_H

#include "lvgl/lvgl.h"

// --- Maszyna stanów aplikacji ---
typedef enum {
    APP_HUB,
    APP_CHROMA_MIDI,
    APP_CHROMA_RSVP,
    APP_CHROMA_SAND,
    APP_CHROMA_CONTROL,
    APP_DRIFTONE
} achromatosis_app_t;

// --- Wymiary ekranu Waveshare ---
#define DISPLAY_WIDTH             172
#define DISPLAY_HEIGHT            640

// --- Kolorystyka (Skala szarości) ---
#define ACHROM_COLOR_BG                 0xDCDCDC
#define ACHROM_COLOR_SHADOW_DARK        0xB8B8B8
#define ACHROM_COLOR_SHADOW_LIGHT       0xFFFFFF
#define ACHROM_COLOR_TEXT               0x282828

#define ACHROM_ROW0_PILL_TOP            0xD4D4D4
#define ACHROM_ROW1_PILL_TOP            0xE4E4E4
#define ACHROM_ROW1_PILL_BOT            0xC8C8C8
#define ACHROM_ROW2_PILL_TOP            0xD8D8D8
#define ACHROM_ROW2_PILL_BOT            0xB8B8B8
#define ACHROM_ROW3_PILL_TOP            0xC8C8C8
#define ACHROM_ROW3_PILL_BOT            0xA8A8A8

#define ACHROM_FONT_MAIN                (&lv_font_montserrat_14)

// --- Geometria Neumorfizmu ---
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

#endif // ACHROMATOSIS_CONFIG_H