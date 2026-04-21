#include <stdio.h>
#include "delay.h"
#include "esp_rom_sys.h"
//Libreria segun https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/internal-unstable.html

void delay_ms(uint32_t ms)
{
    esp_rom_delay_us(ms * 1000);//1ms = 1000microsS
}

void delay_s(uint32_t s)
{
    esp_rom_delay_us(s * 1000000);//1s= 1000000 microsegundos
}