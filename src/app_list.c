#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "app_list.h"

static sys_slist_t app_list = SYS_SLIST_STATIC_INIT(&app_list);

int app_list_list(void)
{
    sys_snode_t *node = sys_slist_peek_head(&app_list);
    int i = 0;
    while (node != NULL) {
        app_node_t *app_node = CONTAINER_OF(node, app_node_t, node);
        printk("App Index: %d, Name: %s, Priority: %d\n", i, app_node->app->name, app_node->app->prio);
        node = sys_slist_peek_next(node);
        i++;
    }
    return 0;
}

int app_list_insert(const app_t *app)
{
    if (app == NULL) {
        return -EINVAL;
    }

    sys_snode_t *prev = NULL;
    sys_snode_t *node = sys_slist_peek_head(&app_list);

    app_node_t *app_node = k_malloc(sizeof(app_node_t));
    if (app_node == NULL) {
        printk("Failed to allocate memory for app node\n");
        return -ENOMEM;
    }
    app_node->app = app;

    while (node != NULL) {
        app_node_t *curr = CONTAINER_OF(node, app_node_t, node);
        if (curr->app->prio > app->prio) {
            sys_slist_insert(&app_list, prev, &app_node->node);
            return 0;
        }
        prev = node;
        node = sys_slist_peek_next(node);
    }
    
    sys_slist_append(&app_list, &app_node->node);
    return 0;
}

const app_t *app_list_get(int index)
{
    int i = 0;
    sys_snode_t *node = sys_slist_peek_head(&app_list);
    while (node != NULL) {
        if (i == index) {
            app_node_t *app_node = CONTAINER_OF(node, app_node_t, node);
            return app_node->app;
        }
        node = sys_slist_peek_next(node);
        i++;
    }
    return NULL;
}
