#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <stdio.h>
#include "prefs.h"

LOG_MODULE_REGISTER(prefs, CONFIG_APP_MGR_LOG_LEVEL);

#define PREFS_ROOT "cube"

typedef struct {
    void *buf;
    size_t len;
    int ret;
} prefs_read_t;

static int prefs_ready(void)
{
    static int init_ret = 1;

    if (init_ret == 1) {
        init_ret = settings_subsys_init();
        if (init_ret < 0) {
            LOG_ERR("Settings init failed (%d)", init_ret);
        }
    }
    return init_ret;
}

static int read_cb(const char *key, size_t len, settings_read_cb read_fn, void *cb_arg, void *param)
{
    prefs_read_t *r = param;

    // 只要剛好是這個 key 本身，不要底下的子 key
    if (key != NULL) {
        return 0;
    }
    r->ret = read_fn(cb_arg, r->buf, MIN(len, r->len));
    return 1;
}

int prefs_load(const char *key, void *buf, size_t len)
{
    char name[SETTINGS_MAX_NAME_LEN + 1];
    prefs_read_t r = {
        .buf = buf,
        .len = len,
        .ret = -ENOENT,
    };

    if (prefs_ready() < 0) {
        return -EIO;
    }
    snprintf(name, sizeof(name), PREFS_ROOT "/%s", key);
    int ret = settings_load_subtree_direct(name, read_cb, &r);
    return ret < 0 ? ret : r.ret;
}

int prefs_save(const char *key, const void *buf, size_t len)
{
    char name[SETTINGS_MAX_NAME_LEN + 1];

    if (prefs_ready() < 0) {
        return -EIO;
    }
    snprintf(name, sizeof(name), PREFS_ROOT "/%s", key);
    int ret = settings_save_one(name, buf, len);
    if (ret < 0) {
        LOG_ERR("Failed to save %s (%d)", name, ret);
    }
    return ret;
}
