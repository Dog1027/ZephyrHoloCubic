#pragma once
#include "app.h"

typedef enum {
    APP_MGR_EVENT_NONE,
    APP_MGR_EVENT_APP_LOAD,
    APP_MGR_EVENT_APP_END,
} app_mgr_event_t;

int app_mgr_check(app_mgr_event_t *p_event, const void **p_data);
int app_mgr_load_app(int index);
int app_mgr_end_app(void);
