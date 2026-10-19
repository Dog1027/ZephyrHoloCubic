#pragma once

typedef struct {
    int prio;
    const char *name;
    const char *icon;
    void (*init)(void *scr);
    void (*run)(void *scr);
    void (*exit)(void *scr);
} app_t;

#define APP_INST_CREATE(_inst, _name, _icon, _init, _run, _exit, _prio) \
    static void _init(void *); \
    static void _run(void *); \
    static const app_t _inst = { \
        .prio = _prio , \
        .name = #_name, \
        .icon = _icon, \
        .init = (void (*)(void *))_init, \
        .run = (void (*)(void *))_run, \
        .exit = (void (*)(void *))_exit, \
    }

#define APP_REGISTER(_name, _icon, _init, _run, _exit, _prio) \
    APP_INST_CREATE(app_inst, _name, _icon, _init, _run, _exit, _prio); \
    int app_list_insert(const app_t *app); \
    static int app_register(void) \
    { \
        const app_t *app = &app_inst; \
        app_list_insert(app); \
        printk("Registered app: %s with priority %d\n", app->name, app->prio); \
        return 0; \
    } \
    SYS_INIT(app_register, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY)
