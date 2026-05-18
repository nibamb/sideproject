#ifndef UI_LAUNCHER_H
#define UI_LAUNCHER_H

#include "achromatosis_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*launcher_app_select_cb)(achromatosis_app_t app);

// Dostosowane do Twojego pliku nagłówkowego - przyjmuje callback bezpośrednio w init
void ui_launcher_init(launcher_app_select_cb callback);
void ui_launcher_register_callback(launcher_app_select_cb cb);
void ui_launcher_show(void);
void ui_launcher_hide(void);

#ifdef __cplusplus
}
#endif

#endif // UI_LAUNCHER_H