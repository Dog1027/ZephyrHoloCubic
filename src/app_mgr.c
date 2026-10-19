#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "app_list.h"
#include "app_mgr.h"

LOG_MODULE_REGISTER(app_mgr, CONFIG_APP_MGR_LOG_LEVEL);

#define APP_MGR_QUEUE_LEN 4

typedef struct {
    app_mgr_event_t event;
    const void *data;
} app_mgr_t;

// 佇列：連續送出的事件 (例如換 app 後馬上結束) 會依序處理，不會互相覆蓋
K_MSGQ_DEFINE(app_mgr_q, sizeof(app_mgr_t), APP_MGR_QUEUE_LEN, 4);

static int app_mgr_send(app_mgr_event_t event, const void *data)
{
    app_mgr_t msg = {
        .event = event,
        .data = data,
    };

    int ret = k_msgq_put(&app_mgr_q, &msg, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("App manager queue full, event %d dropped", event);
    }
    return ret;
}

int app_mgr_check(app_mgr_event_t *p_event, const void **p_data)
{
    app_mgr_t msg;

    if (p_event == NULL || p_data == NULL) {
        return -EINVAL;
    }

    int ret = k_msgq_get(&app_mgr_q, &msg, K_NO_WAIT);
    if (ret < 0) {
        return ret;
    }

    *p_event = msg.event;
    *p_data = msg.data;
    LOG_DBG("App manager event checked: %d", *p_event);
    return 0;
}

int app_mgr_load_app(int index)
{
    const app_t *app = app_list_get(index);
    if (app == NULL) {
        LOG_ERR("Application with index %d not found", index);
        return -ENOENT;
    }

    int ret = app_mgr_send(APP_MGR_EVENT_APP_LOAD, app);
    if (ret == 0) {
        LOG_DBG("Loaded application: %s", app->name);
    }
    return ret;
}

int app_mgr_end_app(void)
{
    int ret = app_mgr_send(APP_MGR_EVENT_APP_END, NULL);
    if (ret == 0) {
        LOG_DBG("Ended foreground application");
    }
    return ret;
}

static int app_mgr_init(void)
{
    int ret = app_mgr_load_app(0);
    if (ret < 0) {
        LOG_ERR("Failed to load initial application");
        return ret;
    }

    LOG_INF("Application Manager Initialized");
    return 0;
}

SYS_INIT(app_mgr_init, APPLICATION, CONFIG_APP_MGR_INIT_PRIORITY);
