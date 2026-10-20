#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include "app_mgr.h"
#include "app_list.h"
#include "nav.h"
#include <string.h>
#include <strings.h>
#include <zephyr/drivers/sensor.h>
#include "led.h"

/* ---------- app: list / open / close ---------- */

static int _app_list(const struct shell *shell, size_t argc, char **argv)
{
    const app_t *app;
    for (int i = 0; (app = app_list_get(i)) != NULL; i++) {
        shell_print(shell, "%2d  %s", i, app->name);
    }
    return 0;
}

// 名稱 (不分大小寫) 或 index 都可以，Menu (index 0) 等於關掉前景 app
static int _app_open(const struct shell *shell, size_t argc, char **argv)
{
    char *end;
    long index = strtol(argv[1], &end, 10);

    if (*end != '\0') {
        const app_t *app;
        index = -1;
        for (int i = 0; (app = app_list_get(i)) != NULL; i++) {
            if (strcasecmp(app->name, argv[1]) == 0) {
                index = i;
                break;
            }
        }
    }

    if (index < 0 || app_list_get(index) == NULL) {
        shell_error(shell, "No app '%s', see: app list", argv[1]);
        return -ENOENT;
    }
    return index == 0 ? app_mgr_end_app() : app_mgr_load_app(index);
}

static int _app_close(const struct shell *shell, size_t argc, char **argv)
{
    return app_mgr_end_app();
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_app,
    SHELL_CMD_ARG(list, NULL, "List apps with their index", _app_list, 1, 0),
    SHELL_CMD_ARG(open, NULL, "Open an app: app open <name|index>", _app_open, 2, 0),
    SHELL_CMD_ARG(close, NULL, "Close the foreground app, back to the menu", _app_close, 1, 0),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(app, &sub_app, "App commands", NULL);

/* ---------- ui: navigation ---------- */

static int _ui_next(const struct shell *shell, size_t argc, char **argv)
{
    return nav_send(NAV_NEXT);
}

static int _ui_prev(const struct shell *shell, size_t argc, char **argv)
{
    return nav_send(NAV_PREV);
}

static int _ui_ok(const struct shell *shell, size_t argc, char **argv)
{
    return nav_send(NAV_OK);
}

static int _ui_back(const struct shell *shell, size_t argc, char **argv)
{
    return nav_send(NAV_BACK);
}

// 跟敲一下裝置完全相同，方便測試或不想真的敲的時候用
static int _ui_tap(const struct shell *shell, size_t argc, char **argv)
{
    return nav_send(NAV_TAP);
}

#define REMOTE_ESC_TIMEOUT_MS 50

// 遙控模式的跳脫序列狀態 0: 一般, 1: 收到 ESC, 2: 收到 ESC [ (方向鍵)
static atomic_t esc_state;

// 方向鍵是 ESC [ X 三個字元，但序列埠可能分好幾次送到；ESC 後面一段時間沒有 [ 才算按了 Esc
static void esc_timeout(struct k_work *work)
{
    if (atomic_cas(&esc_state, 1, 0)) {
        nav_send(NAV_BACK);
    }
}
static K_WORK_DELAYABLE_DEFINE(esc_work, esc_timeout);

// 遙控模式：shell 進入 bypass，每個按鍵直接變成導航指令
static void remote_bypass(const struct shell *shell, uint8_t *data, size_t len)
{
    k_work_cancel_delayable(&esc_work);

    for (size_t i = 0; i < len; i++) {
        uint8_t c = data[i];

        if (atomic_get(&esc_state) == 1) {
            atomic_set(&esc_state, 0);
            if (c == '[') {
                atomic_set(&esc_state, 2);
                continue;
            }
            nav_send(NAV_BACK);
        }
        else if (atomic_get(&esc_state) == 2) {
            atomic_set(&esc_state, 0);
            if (c == 'A' || c == 'D') {
                nav_send(NAV_PREV);
            }
            else if (c == 'B' || c == 'C') {
                nav_send(NAV_NEXT);
            }
            continue;
        }

        switch (c) {
        case 0x1b:
            atomic_set(&esc_state, 1);
            break;
        case '\r':
        case ' ':
            nav_send(NAV_OK);
            break;
        case 'w':
        case 'a':
            nav_send(NAV_PREV);
            break;
        case 's':
        case 'd':
            nav_send(NAV_NEXT);
            break;
        case 'e':
            nav_send(NAV_TAP);
            break;
        case 'q':
        case 'b':
        case 0x7f:
        case 0x08:
            nav_send(NAV_BACK);
            break;
        case 0x11:  // Ctrl+Q
        case 0x03:  // Ctrl+C
            atomic_set(&esc_state, 0);
            shell_set_bypass(shell, NULL);
            shell_print(shell, "Remote mode off");
            return;
        default:
            break;
        }
    }

    if (atomic_get(&esc_state) == 1) {
        k_work_schedule(&esc_work, K_MSEC(REMOTE_ESC_TIMEOUT_MS));
    }
}

static int _ui_remote(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "Remote mode: arrows or w/a/s/d move, Enter/Space = OK, "
                       "e = tap, q/Esc/Backspace/b = back to menu, Ctrl+Q or Ctrl+C to quit");
    shell_set_bypass(shell, remote_bypass);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ui,
    SHELL_CMD_ARG(next, NULL, "Highlight the next item", _ui_next, 1, 0),
    SHELL_CMD_ARG(prev, NULL, "Highlight the previous item", _ui_prev, 1, 0),
    SHELL_CMD_ARG(ok, NULL, "Select the highlighted item", _ui_ok, 1, 0),
    SHELL_CMD_ARG(back, NULL, "Close the foreground app", _ui_back, 1, 0),
    SHELL_CMD_ARG(tap, NULL, "Same as tapping the cube", _ui_tap, 1, 0),
    SHELL_CMD_ARG(remote, NULL, "Control with single key presses", _ui_remote, 1, 0),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ui, &sub_ui, "Navigation commands", NULL);

/* ---------- tool: hardware ---------- */

static const struct device *imu_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(imu));
static int _imu_get(const struct shell *shell, size_t argc, char **argv)
{
    if (imu_dev == NULL || !device_is_ready(imu_dev)) {
        shell_print(shell, "IMU device not ready");
        return -ENODEV;
    }

    struct sensor_value accel[3];
    if (sensor_sample_fetch(imu_dev) < 0) {
        shell_print(shell, "Failed to fetch sample from IMU");
        return -EIO;
    }

    if (sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) {
        shell_print(shell, "Failed to get accel data from IMU");
        return -EIO;
    }

    // val1 / val2 的正負號各自獨立 (-0.12 是 val1 = 0, val2 = -124499)，換成毫單位再印
    static const char axis[] = {'X', 'Y', 'Z'};
    for (int i = 0; i < 3; i++) {
        int32_t milli = (int32_t)sensor_value_to_milli(&accel[i]);
        shell_print(shell, "Accel %c: %s%d.%03d m/s^2", axis[i], milli < 0 ? "-" : "",
                    abs(milli) / 1000, abs(milli) % 1000);
    }

    return 0;
}

static int _led_set(const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 4) {
        shell_print(shell, "Usage: tool led_set <r> <g> <b>");
        return -EINVAL;
    }

    uint8_t r = (uint8_t)atoi(argv[1]);
    uint8_t g = (uint8_t)atoi(argv[2]);
    uint8_t b = (uint8_t)atoi(argv[3]);

    int ret = led_set_raw(r, g, b);
    if (ret != 0) {
        shell_print(shell, "Failed to set LED color, error: %d", ret);
        return ret;
    }

    shell_print(shell, "Set LED color to R:%d G:%d B:%d", r, g, b);
    return 0;
}

/* Define shell subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_tool,
    SHELL_CMD_ARG(imu_get, NULL, "get imu data", _imu_get, 1, 0),
    SHELL_CMD_ARG(led_set, NULL, "set led color", _led_set, 4, 0),
    SHELL_SUBCMD_SET_END);

/* Register main shell command */
SHELL_CMD_ARG_REGISTER(tool, &sub_tool, "Hardware commands", NULL, 1, 0);
