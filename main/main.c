
#include <stdio.h>
#include "rgb_led.h"
#include "delay.h"
#include <unistd.h> // Librería estándar para sleep/usleep

#define BLINK_PERIOD_MS 500

void app_main(void)
{
    rgb_led_init();
    while (1)
    {
        printf("Rojo\n");
        rgb_led_set_color(255, 0, 0);
        delay_ms(BLINK_PERIOD_MS);

        printf("Verde\n");
        rgb_led_set_color(0, 255, 0);
        delay_ms(BLINK_PERIOD_MS);

        printf("Azul\n");
        rgb_led_set_color(0, 0, 255);
        delay_ms(BLINK_PERIOD_MS);

        printf("Apagado\n");
        rgb_led_off();
        delay_ms(BLINK_PERIOD_MS);
    }
}