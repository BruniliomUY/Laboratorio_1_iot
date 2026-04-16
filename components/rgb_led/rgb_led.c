#include "rgb_led.h"
#include "esp_log.h"

static const char *TAG = "RGB_LIB";
led_strip_t *p_strip = NULL;

// Aquí incluyes la función de inicialización que ya tienes (led_rgb_init)
// pero adaptada para guardar el puntero en p_strip.
esp_err_t rgb_led_init(void) {
    // ... (aquí va toda la lógica de rmt_tx_channel_config_t y rmt_new_bytes_encoder)
    // Asegúrate de asignar el resultado a p_strip
    return ESP_OK; 
}

void rgb_led_set_color(uint32_t red, uint32_t green, uint32_t blue) {
    if (p_strip) {
        p_strip->set_pixel(p_strip, 0, red, green, blue);
        p_strip->refresh(p_strip, 100);
    }
}

void rgb_led_clear(void) {
    if (p_strip) {
        p_strip->clear(p_strip, 100);
    }
}