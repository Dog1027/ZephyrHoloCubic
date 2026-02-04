#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include "input.h"

LOG_MODULE_REGISTER(input, CONFIG_INPUT_LOG_LEVEL);

#define ALPHA 0.98f        // 濾波係數
#define SAMPLING_PERIOD_MS 10  // 取樣週期 (毫秒)
#define RAD_TO_DEG 57.29578f // 弧度轉角度係數

static void input_thread_entry(void *arg1, void *arg2, void *arg3);

K_THREAD_DEFINE(
    input_thread,
    0x2000,
    input_thread_entry,
    NULL, NULL, NULL,
    7,
    0,
    0
);

K_MUTEX_DEFINE(input_mutex);

static const struct device *imu_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(imu));
static float roll_angle = 0.0f;

char input_press(void)
{
    return 0;
}

char input_focus(int obj_count, void (*focus)(int obj_index))
{
    if (obj_count == 0) return -1;

    // 定義總範圍 (150度)
    const float total_range = 100.0f;
    float sector_size = total_range / obj_count;

    k_mutex_lock(&input_mutex, K_FOREVER);
    int target_index = (int)((roll_angle + 90 - (180 - total_range) / 2) / sector_size);
    k_mutex_unlock(&input_mutex);
    
    if (target_index < 0) target_index = 0;
    if (target_index >= obj_count) target_index = obj_count - 1;

    LOG_INF("Target Index: %d, n: %d", target_index, obj_count);

    static int last_target_index = -1;
    if (target_index != last_target_index) {
        last_target_index = target_index;
        focus(target_index);
        
        // 可選：觸發一個短震動
        // trigger_haptic_feedback(); 
    }

    return 0;
}

void input_thread_entry(void *arg1, void *arg2, void *arg3)
{
    struct sensor_value accel[3];
    struct sensor_value gyro[3];

    if (imu_dev == NULL || !device_is_ready(imu_dev)) {
        LOG_ERR("IMU device not ready");
        while (1) {
            k_msleep(10000);
        }
    }

    if (sensor_sample_fetch(imu_dev) == 0 &&
        sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel) == 0 &&
        sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro) == 0) {
        float ay = (float)sensor_value_to_double(&accel[1]);
        float az = (float)sensor_value_to_double(&accel[2]);
        float gx = (float)sensor_value_to_double(&gyro[0]);
        
        float accel_roll = atan2f(ay, az) * RAD_TO_DEG;
        roll_angle = accel_roll;
    }

    while (1) {
        if (sensor_sample_fetch(imu_dev) < 0) {
            LOG_ERR("Failed to fetch sample from IMU");
            k_msleep(SAMPLING_PERIOD_MS);
            continue;
        }

        if (sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel) < 0) {
            LOG_ERR("Failed to get accel data from IMU");
            k_msleep(SAMPLING_PERIOD_MS);
            continue;
        }

        if (sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro) < 0) {
            LOG_ERR("Failed to get gyro data from IMU");
            k_msleep(SAMPLING_PERIOD_MS);
            continue;
        }

        float ax = (float)sensor_value_to_double(&accel[0]);
        float ay = (float)sensor_value_to_double(&accel[1]);
        float az = (float)sensor_value_to_double(&accel[2]);
        float gx = (float)sensor_value_to_double(&gyro[0]); // 繞 X 軸角速度 (rad/s)

        // 計算加速度計提供的「絕對角度」(參考值)
        float accel_roll = atan2f(ay, az) * RAD_TO_DEG;

        // 互補濾波：融合陀螺儀積分與加速度計角度
        // 注意：Zephyr 讀出的陀螺儀單位通常是 rad/s，要轉成 deg/s
        float gx_deg = gx * RAD_TO_DEG;


        k_mutex_lock(&input_mutex, K_FOREVER);
        roll_angle = ALPHA * (roll_angle + gx_deg * (SAMPLING_PERIOD_MS / 1000.0f)) + (1.0f - ALPHA) * accel_roll;
        k_mutex_unlock(&input_mutex);

        // int roll_int = (int)(roll_angle);
        // int roll_frac = (int)(fabsf(roll_angle - roll_int) * 100.0f); // 小數部分
        // LOG_INF("Roll Angle: %d.%02d deg", roll_int, roll_frac);

        k_msleep(SAMPLING_PERIOD_MS);
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
