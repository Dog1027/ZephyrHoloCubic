#pragma once

#include <zephyr/sys/slist.h>
#include "app.h"

typedef struct {
    sys_snode_t node;
    const app_t *app;
} app_node_t;

int app_list_insert(const app_t *app);
const app_t *app_list_get(int index);
