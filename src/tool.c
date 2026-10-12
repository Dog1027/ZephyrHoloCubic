#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include "app_mgr.h"
#include "input.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/led_strip.h>
#include <lvgl.h>

static int _fg(const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2) {
        shell_print(shell, "Usage: tool fg <index>");
        return -EINVAL;
    }

    int index = atoi(argv[1]);
    if (index < 1) {
        shell_print(shell, "Index must larger than 0");
        return -EINVAL;
    }

    int ret = app_mgr_load_app(index);
    if (ret != 0) {
        shell_print(shell, "Failed to load foreground app with index %d, error: %d", index, ret);
        return ret;
    }

    shell_print(shell, "Foreground app with index %d loaded", index);
    return 0;
}

static int _end_fg(const struct shell *shell, size_t argc, char **argv)
{
    int ret = app_mgr_end_app();
    if (ret != 0) {
        shell_print(shell, "Failed to end foreground app, error: %d", ret);
        return ret;
    }

    shell_print(shell, "Foreground app ended");
    return 0;
}

static int _list(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "Not implemented yet");
    return 0;
}

static int _get_char(const struct shell *shell, size_t argc, char **argv)
{
    // char key = input_get();
    // if (key == '\0') {
    //     shell_print(shell, "No input available");
    // }
    // else {
    //     shell_print(shell, "Got input char: %c", key);
    // }
    return 0;
}

static int _put_char(const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2) {
        shell_print(shell, "Usage: tool putchar <char>");
        return -EINVAL;
    }

    char key = argv[1][0];
    // input_put(key);
    shell_print(shell, "Put input char: %c", key);
    return 0;
}

static int _next(const struct shell *shell, size_t argc, char **argv)
{
    lv_group_t *main_group = lv_group_get_default();
    lv_group_focus_next(main_group);
    return 0;
}

static int _prev(const struct shell *shell, size_t argc, char **argv)
{
    lv_group_t *main_group = lv_group_get_default();
    lv_group_focus_prev(main_group);
    return 0;
}

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

    shell_print(shell, "Accel X: %d.%06d m/s^2", accel[0].val1, accel[0].val2);
    shell_print(shell, "Accel Y: %d.%06d m/s^2", accel[1].val1, accel[1].val2);
    shell_print(shell, "Accel Z: %d.%06d m/s^2", accel[2].val1, accel[2].val2);

    return 0;
}

static const struct device *led_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(rgb_led));
static int _led_set(const struct shell *shell, size_t argc, char **argv)
{
    if (led_dev == NULL || !device_is_ready(led_dev)) {
        shell_print(shell, "LED strip device not ready");
        return -ENODEV;
    }

    if (argc != 4) {
        shell_print(shell, "Usage: tool led_set <r> <g> <b>");
        return -EINVAL;
    }

    uint8_t r = (uint8_t)atoi(argv[1]);
    uint8_t g = (uint8_t)atoi(argv[2]);
    uint8_t b = (uint8_t)atoi(argv[3]);

    struct led_rgb pixel = { .r = r, .g = g, .b = b };
    int ret = led_strip_update_rgb(led_dev, &pixel, 1);
    if (ret != 0) {
        shell_print(shell, "Failed to set LED color, error: %d", ret);
        return ret;
    }

    shell_print(shell, "Set LED color to R:%d G:%d B:%d", r, g, b);
    return 0;
}

/* Define shell subcommands */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_tool,
    SHELL_CMD_ARG(fg, NULL, "fg", _fg, 2, 0),
    SHELL_CMD_ARG(end_fg, NULL, "end_fg", _end_fg, 1, 0),
    SHELL_CMD_ARG(list, NULL, "list", _list, 1, 0),
    SHELL_CMD_ARG(getchar, NULL, "get char", _get_char, 1, 0),
    SHELL_CMD_ARG(putchar, NULL, "put char", _put_char, 2, 0),
    SHELL_CMD_ARG(next, NULL, "next", _next, 1, 0),
    SHELL_CMD_ARG(prev, NULL, "prev", _prev, 1, 0),
    SHELL_CMD_ARG(imu_get, NULL, "get imu data", _imu_get, 1, 0),
    SHELL_CMD_ARG(led_set, NULL, "set led color", _led_set, 4, 0),
    SHELL_SUBCMD_SET_END);

/* Register main shell command */
SHELL_CMD_ARG_REGISTER(tool, &sub_tool, "Tool commands", NULL, 1, 0);
