#pragma once

#include <zephyr/kernel.h>
#include "app.h"
#include "lvgl.h"

// Id 0 reserved for backgriund app registration
#define APP_FG_REGISTER(_name, _init, _run, _prio) \
    BUILD_ASSERT((_prio) != 0, "Foreground app priority cannot be 0"); \
    APP_REGISTER(_name, _init, _run, _prio)
