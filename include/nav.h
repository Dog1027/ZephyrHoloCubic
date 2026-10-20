#pragma once

#include <stdbool.h>

/*
 * Navigation commands. Any thread (shell, input) queues them with nav_send(),
 * the app thread applies them to LVGL in nav_process(), so LVGL is only touched
 * from one thread.
 */
typedef enum {
    NAV_NEXT,   // move the highlight forward
    NAV_PREV,   // move the highlight back
    NAV_OK,     // click the highlighted object
    NAV_BACK,   // close the foreground app
    NAV_TAP,    // a tap on the cube: like NAV_NEXT inside an app unless it handles it, ignored in the menu
} nav_cmd_t;

// Return true when the command was handled, false to fall back to the default action
typedef bool (*nav_handler_t)(nav_cmd_t cmd);

// Queue a command, safe from any thread
int nav_send(nav_cmd_t cmd);

// Let the foreground app see NEXT / PREV / OK / TAP first, e.g. Photos turns NEXT into
// "next photo". Call it from the app's init; it is cleared automatically when the app closes.
void nav_set_handler(nav_handler_t handler);

// App thread only: apply queued commands to the default group.
// in_app is false while the menu is shown, taps are ignored there.
void nav_process(bool in_app);
