#pragma once

typedef struct {
    float roll;
    float pitch;
    float yaw;
} input_angle_t;

char input_get(input_angle_t *angle);
char input_press(void);
char input_focus(int obj_count, void (*focus)(int obj_index));
