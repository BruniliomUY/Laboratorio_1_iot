#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>

// Inicializa el LED 
void rgb_led_init(void);

// Setea un color RGB (0–255 cada componente)
void rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

// Apaga el LED
void rgb_led_off(void);

#endif 
