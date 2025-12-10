#include <stdio.h>
#include <zephyr/kernel.h>

int main(void)
{
    printk("Starting main application...\n");
    while (1) {
        // Infinite loop
        k_sleep(K_MSEC(1000));
    }
    return 0;
}
