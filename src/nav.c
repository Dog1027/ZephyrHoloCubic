#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include "app_mgr.h"
#include "nav.h"

LOG_MODULE_REGISTER(nav, CONFIG_APP_THREAD_LOG_LEVEL);

#define NAV_QUEUE_LEN 8

K_MSGQ_DEFINE(nav_q, sizeof(nav_cmd_t), NAV_QUEUE_LEN, 4);

static nav_handler_t handler;

int nav_send(nav_cmd_t cmd)
{
    int ret = k_msgq_put(&nav_q, &cmd, K_NO_WAIT);
    if (ret < 0) {
        LOG_WRN("Navigation queue full, command %d dropped", cmd);
    }
    return ret;
}

void nav_set_handler(nav_handler_t new_handler)
{
    handler = new_handler;
}

static void nav_apply(nav_cmd_t cmd)
{
    lv_group_t *g = lv_group_get_default();

    if (cmd == NAV_BACK) {
        app_mgr_end_app();
        return;
    }

    if (handler != NULL && handler(cmd)) {
        return;
    }
    if (g == NULL) {
        return;
    }

    switch (cmd) {
    case NAV_NEXT:
    case NAV_TAP:
        lv_group_focus_next(g);
        break;
    case NAV_PREV:
        lv_group_focus_prev(g);
        break;
    case NAV_OK: {
        lv_obj_t *focused = lv_group_get_focused(g);
        if (focused != NULL) {
            lv_obj_send_event(focused, LV_EVENT_CLICKED, NULL);
        }
        break;
    }
    default:
        break;
    }
}

void nav_process(void)
{
    nav_cmd_t cmd;

    while (k_msgq_get(&nav_q, &cmd, K_NO_WAIT) == 0) {
        LOG_DBG("Nav command %d", cmd);
        nav_apply(cmd);
    }
}
