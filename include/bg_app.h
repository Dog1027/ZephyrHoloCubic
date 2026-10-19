#pragma once

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include "app_mgr.h"

#define APP_BG_REGISTER(_name, _app_init, _app_run) \
    LOG_MODULE_REGISTER(_name, CONFIG_APP_BG_LOG_LEVEL); \
    APP_REGISTER(_name, NULL, _app_init, _app_run, NULL, 0)
