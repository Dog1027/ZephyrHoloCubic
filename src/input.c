#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <string.h>
#include "input.h"
#include "nav.h"

LOG_MODULE_REGISTER(input, CONFIG_INPUT_LOG_LEVEL);

#define ALPHA 0.98f        // 濾波係數
#define SAMPLING_PERIOD_MS 10  // 取樣週期 (毫秒)
#define RAD_TO_DEG 57.29578f // 弧度轉角度係數
#define TAP_THRESHOLD 8.0f   // 敲擊判定：相鄰兩次取樣的加速度變化量 (m/s^2)
#define TAP_COOLDOWN_MS 300  // 敲擊後的冷卻時間，避免一次敲擊觸發多次

static void input_thread_entry(void *arg1, void *arg2, void *arg3);

K_THREAD_DEFINE(
    input_thread,
    CONFIG_INPUT_THREAD_STACK_SIZE,
    input_thread_entry,
    NULL, NULL, NULL,
    7,
    0,
    0
);

K_MUTEX_DEFINE(input_mutex);

static const struct device *imu_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(imu));
static input_angle_t current_angle = {0.0f, 0.0f, 0.0f};


// 對外提供的角度，依螢幕鏡像 / IMU 安裝方向修正正負號 (內部濾波一律用感測器原始方向)
static input_angle_t angle_out(void)
{
    k_mutex_lock(&input_mutex, K_FOREVER);
    input_angle_t angle = current_angle;
    k_mutex_unlock(&input_mutex);

    if (IS_ENABLED(CONFIG_INPUT_ROLL_INVERT)) {
        angle.roll = -angle.roll;
    }
    if (IS_ENABLED(CONFIG_INPUT_PITCH_INVERT)) {
        angle.pitch = -angle.pitch;
    }
    return angle;
}

char input_get(input_angle_t *angle)
{
    *angle = angle_out();
    return 0;
}

static int imu_read(float accel[3], float gyro[3])
{
    struct sensor_value a[3];
    struct sensor_value g[3];

    if (sensor_sample_fetch(imu_dev) < 0) {
        LOG_ERR("Failed to fetch sample from IMU");
        return -EIO;
    }

    if (sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, a) < 0) {
        LOG_ERR("Failed to get accel data from IMU");
        return -EIO;
    }

    if (sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, g) < 0) {
        LOG_ERR("Failed to get gyro data from IMU");
        return -EIO;
    }

    for (int i = 0; i < 3; i++) {
        accel[i] = (float)sensor_value_to_double(&a[i]); // m/s^2
        gyro[i] = (float)sensor_value_to_double(&g[i]);  // rad/s
    }
    return 0;
}

// 換算到 [-180, 180]
static float wrap180(float deg)
{
    while (deg > 180.0f) {
        deg -= 360.0f;
    }
    while (deg < -180.0f) {
        deg += 360.0f;
    }
    return deg;
}

// 由加速度計 (重力方向) 計算的「絕對角度」(參考值)
static void accel_angle(const float accel[3], float *roll, float *pitch)
{
    *roll = atan2f(accel[1], accel[2]) * RAD_TO_DEG;
    *pitch = atan2f(-accel[0], sqrtf(accel[1] * accel[1] + accel[2] * accel[2])) * RAD_TO_DEG;
}

void input_thread_entry(void *arg1, void *arg2, void *arg3)
{
    float accel[3];
    float gyro[3];
    float prev_accel[3];
    float accel_roll;
    float accel_pitch;
    int64_t tap_cooldown_end = 0;
    const float dt = SAMPLING_PERIOD_MS / 1000.0f;

    if (imu_dev == NULL || !device_is_ready(imu_dev)) {
        LOG_ERR("IMU device not ready");
        while (1) {
            k_msleep(10000);
        }
    }

    while (imu_read(accel, gyro) < 0) {
        k_msleep(SAMPLING_PERIOD_MS);
    }
    accel_angle(accel, &accel_roll, &accel_pitch);
    current_angle.roll = accel_roll;
    current_angle.pitch = accel_pitch;
    memcpy(prev_accel, accel, sizeof(accel));

    while (1) {
        k_msleep(SAMPLING_PERIOD_MS);

        if (imu_read(accel, gyro) < 0) {
            continue;
        }

        // 敲擊偵測：加速度在一次取樣內劇烈變化 (傾斜這類慢動作變化很小)
        float dx = accel[0] - prev_accel[0];
        float dy = accel[1] - prev_accel[1];
        float dz = accel[2] - prev_accel[2];
        float jerk = sqrtf(dx * dx + dy * dy + dz * dz);
        memcpy(prev_accel, accel, sizeof(accel));

        bool spike = jerk > TAP_THRESHOLD;
        int64_t now = k_uptime_get();
        if (spike && now >= tap_cooldown_end) {
            tap_cooldown_end = now + TAP_COOLDOWN_MS;
            // 敲一下當成一個導航指令，預設等於「下一個」，app 可以自己接手 (例如 Dice 擲骰)
            nav_send(NAV_TAP);
            LOG_DBG("Tap detected, jerk: %d m/s^2", (int)jerk);
        }

        accel_angle(accel, &accel_roll, &accel_pitch);

        // 互補濾波：融合陀螺儀積分與加速度計角度
        // 敲擊時的加速度尖峰不代表重力方向，這段期間只用陀螺儀
        float k = spike ? 1.0f : ALPHA;

        k_mutex_lock(&input_mutex, K_FOREVER);
        // roll 在 ±180 度 (螢幕朝下) 會跳號，用角度差融合，不能直接平均 +179 和 -179
        float roll_pred = current_angle.roll + gyro[0] * RAD_TO_DEG * dt;
        current_angle.roll = wrap180(roll_pred + (1.0f - k) * wrap180(accel_roll - roll_pred));
        current_angle.pitch = k * (current_angle.pitch + gyro[1] * RAD_TO_DEG * dt) + (1.0f - k) * accel_pitch;
        k_mutex_unlock(&input_mutex);

    }
}

/*
K_QUEUE_DEFINE(input_queue);

typedef struct {
    void *_;
    char key;
} input_data_t;

char input_get()
{
    char key;
    input_data_t *data;
    data = k_queue_get(&input_queue, K_NO_WAIT);
    if (data == NULL) {
        return '\0';
    }
    key = data->key;
    k_free(data);
    return key;
}

void input_put(char key)
{
    input_data_t *data = k_malloc(sizeof(input_data_t));
    if (data == NULL) {
        return;
    }
    data->key = key;
    k_queue_append(&input_queue, data);
}

*/
