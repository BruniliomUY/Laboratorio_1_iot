// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief LED Strip Type
 */
typedef struct led_strip_s led_strip_t;

/**
 * @brief LED Strip Device Type
 */
typedef void *led_strip_dev_t;

/**
 * @brief LED Strip configuration
 *
 * Se deja igual que la librería vieja.
 * En esta adaptación para IDF 6, "dev" guarda un rmt_channel_handle_t.
 */
typedef struct {
    uint32_t max_leds;
    led_strip_dev_t dev;
} led_strip_config_t;

/**
 * @brief Declare of LED Strip Type
 */
struct led_strip_s {
    esp_err_t (*set_pixel)(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);
    esp_err_t (*refresh)(led_strip_t *strip, uint32_t timeout_ms);
    esp_err_t (*clear)(led_strip_t *strip, uint32_t timeout_ms);
    esp_err_t (*del)(led_strip_t *strip);
};

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config);

/**
 * @brief Inicializa el LED RGB embebido
esp_err_t led_rgb_init(led_strip_t **strip);

#ifdef __cplusplus
}
#endif
