#pragma once

// Orientation from the IMU in degrees. Taps are sent as NAV_TAP (see nav.h).
typedef struct {
    float roll;
    float pitch;
    float yaw;
} input_angle_t;

char input_get(input_angle_t *angle);
