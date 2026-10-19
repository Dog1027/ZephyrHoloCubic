#pragma once

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include "app.h"

// Id 0 reserved for backgriund app registration
// _icon: LV_SYMBOL_* shown in the menu, NULL for a default icon
#define APP_FG_REGISTER(_name, _icon, _init, _run, _exit, _prio) \
    BUILD_ASSERT((_prio) != 0, "Foreground app priority cannot be 0"); \
    LOG_MODULE_REGISTER(_name, CONFIG_APP_FG_LOG_LEVEL); \
    static void _exit(void *); \
    APP_REGISTER(_name, _icon, _init, _run, _exit, _prio)
