#include <stdio.h>
#include "pico/stdlib.h"

const uint led_pin_red = 12;

int main()
{
    uint a = 100;

    gpio_init(led_pin_red);
    gpio_set_dir(led_pin_red, GPIO_OUT);
    stdio_init_all();

    while (true) {
        a++;
        if(a % 2)
            printf("Blinking!\r\n");
        gpio_put(led_pin_red, true);
        sleep_ms(1000);
        gpio_put(led_pin_red, false);
        sleep_ms(1000);
    }
}
