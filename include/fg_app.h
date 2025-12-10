#pragma once

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include "app.h"

// Id 0 reserved for backgriund app registration
#define APP_FG_REGISTER(_name, _init, _run, _prio) \
    BUILD_ASSERT((_prio) != 0, "Foreground app priority cannot be 0"); \
    LOG_MODULE_REGISTER(TEST, CONFIG_APP_FG_LOG_LEVEL); \
    APP_REGISTER(_name, _init, _run, _prio)
