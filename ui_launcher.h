#ifndef UI_LAUNCHER_H
#define UI_LAUNCHER_H

#include "achromatosis_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Typ wskaźnika na funkcję zwrotną (callback) wywoływaną przy wyborze aplikacji
typedef void (*launcher_app_select_cb)(achromatosis_app_t app);

// Inicjalizacja głównego interfejsu menu (launchera)
void ui_launcher_init(void);

// Rejestracja callbacku w maszynie stanów
void ui_launcher_register_callback(launcher_app_select_cb cb);

// Pokazywanie i ukrywanie warstwy launchera
void ui_launcher_show(void);
void ui_launcher_hide(void);

#ifdef __cplusplus
}
#endif

#endif // UI_LAUNCHER_H