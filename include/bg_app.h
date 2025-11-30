#pragma once

#include <zephyr/kernel.h>
#include "app_mgr.h"
#include "lvgl.h"

#define APP_BG_REGISTER(_name, _app_init, _app_run) \
    APP_REGISTER(_name, _app_init, _app_run, 0)
