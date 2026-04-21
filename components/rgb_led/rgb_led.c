#include "rgb_led.h"      
#include "led_strip.h"    
#include "esp_err.h" // lo incluyo para poder verificar errores,de aca saco que ESP_Ok es que salio todo bien https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_err.html
#include <stdint.h>       

#define RGB_TIMEOUT_MS 50 

static led_strip_t *led = NULL; // puntero al LED, arranca en null porque todavia no esta inicializado

void rgb_led_init(void)
{
    if (led != NULL) {    // si ya estaba inicializado no lo vuelvo a inicializar
        return;           
    }

    if (led_rgb_init(&led) != ESP_OK) { // la funcion pide **strip entonces le paso &led (direccion del puntero)
        led = NULL;      // si falla algo dejo NULL para mostrar que no hay led valido inicializado
    }
}

void rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
    if (led == NULL) {    // verifico que el led este inicializado
        return;           
    }

    led->set_pixel(led, 0, red, green, blue); // uso la funcion que esta dentro de la struct. El 0 porque hay un solo led,su indice es 0

    led->refresh(led, RGB_TIMEOUT_MS); // pasa los datos al LED
}

void rgb_led_off(void)
{
    if (led == NULL) {    
        return;           
    }

    led->clear(led, RGB_TIMEOUT_MS); // apaga el LED (lo pone en 0)
}