#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "app_list.h"
#include "app_mgr.h"

LOG_MODULE_REGISTER(app_mgr, CONFIG_APP_MGR_LOG_LEVEL);

typedef struct {
    app_mgr_event_t event;
    const void *data;
} app_mgr_t;

static struct k_mutex app_mgr_mutex;
static struct k_sem app_mgr_sem;

static app_mgr_t _app_mgr_inst = {
    .event = APP_MGR_EVENT_NONE,
    .data = NULL,
};

static app_mgr_t *const _app_mgr = &_app_mgr_inst;

int app_mgr_check(app_mgr_event_t *p_event, const void **p_data)
{
    int ret;

    if (p_event == NULL || p_data == NULL) {
        return -EINVAL;
    }

    ret = k_sem_take(&app_mgr_sem, K_NO_WAIT);
    if (ret < 0) {
        return ret;
    }
    LOG_DBG("App manager semaphore taken");

    ret = k_mutex_lock(&app_mgr_mutex, K_NO_WAIT);
    if (ret < 0) {
        LOG_ERR("Failed to lock app manager mutex");
        return ret;
    }

    *p_event = _app_mgr->event;
    *p_data = _app_mgr->data;
    
    ret = k_mutex_unlock(&app_mgr_mutex);
    if (ret < 0) {
        LOG_ERR("Failed to unlock app manager mutex");
        return ret;
    }

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

    int ret;
    ret = k_mutex_lock(&app_mgr_mutex, K_FOREVER);
    if (ret < 0) {
        LOG_ERR("Failed to lock app manager mutex");
        return ret;
    }

    _app_mgr->event = APP_MGR_EVENT_APP_LOAD;
    _app_mgr->data = app;
    
    ret = k_mutex_unlock(&app_mgr_mutex);
    if (ret < 0) {
        LOG_ERR("Failed to unlock app manager mutex");
        return ret;
    }

    k_sem_give(&app_mgr_sem);
    LOG_DBG("Loaded application: %s", app->name);
    return 0;
}

int app_mgr_end_app(void)
{
    int ret;
    ret = k_mutex_lock(&app_mgr_mutex, K_FOREVER);
    if (ret < 0) {
        LOG_ERR("Failed to lock app manager mutex");
        return ret;
    }

    _app_mgr->event = APP_MGR_EVENT_APP_END;
    _app_mgr->data = NULL;

    ret = k_mutex_unlock(&app_mgr_mutex);
    if (ret < 0) {
        LOG_ERR("Failed to unlock app manager mutex");
        return ret;
    }

    k_sem_give(&app_mgr_sem);
    LOG_DBG("Ended foreground application");
    return 0;
}

static int app_mgr_init(void)
{
    int ret;
    ret = k_sem_init(&app_mgr_sem, 0, 1);
    if (ret < 0) {
        LOG_ERR("Failed to initialize app manager semaphore");
        return ret;
    }
    
    ret = k_mutex_init(&app_mgr_mutex);
    if (ret < 0) {
        LOG_ERR("Failed to initialize app manager mutex");
        return ret;
    }

    ret = app_mgr_load_app(0);
    if (ret < 0) {
        LOG_ERR("Failed to load initial application");
        return ret;
    }

    LOG_INF("Application Manager Initialized");
    return 0;
}

SYS_INIT(app_mgr_init, APPLICATION, CONFIG_APP_MGR_INIT_PRIORITY);
