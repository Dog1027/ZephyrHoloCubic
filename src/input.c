#include "zephyr/kernel.h"

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
