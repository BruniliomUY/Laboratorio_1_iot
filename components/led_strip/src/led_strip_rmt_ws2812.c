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
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "led_strip.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ws2812";
#define STRIP_CHECK(a, str, goto_tag, ret_value, ...)                             \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

#define WS2812_T0H_NS (350)
#define WS2812_T0L_NS (1000)
#define WS2812_T1H_NS (1000)
#define WS2812_T1L_NS (350)
#define WS2812_RESET_US (280)

#define KALUGA_RGB_LED_PIN GPIO_NUM_45
#define KALUGA_RGB_LED_NUMBER 1

uint32_t ws2812_t0h_ticks = 0;
uint32_t ws2812_t1h_ticks = 0;
uint32_t ws2812_t0l_ticks = 0;
uint32_t ws2812_t1l_ticks = 0;

led_strip_t *embedded_led;

typedef struct 
{
    led_strip_t parent;
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t encoder;   // 👈 AGREGAR
    uint32_t strip_len;
    uint8_t buffer[0];
} ws2812_t;

/**
 * @brief Conver RGB data to RMT format.
 *
 * @note For WS2812, R,G,B each contains 256 different choices (i.e. uint8_t)
 *
 * @param[in] src: source data, to converted to RMT format
 * @param[in] dest: place where to store the convert result
 * @param[in] src_size: size of source data
 * @param[in] wanted_num: number of RMT items that want to get
 * @param[out] translated_size: number of source data that got converted
 * @param[out] item_num: number of RMT items which are converted from source data
 */
esp_err_t ws2812_set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    ESP_LOGI(TAG, "%s", __func__);

    esp_err_t ret = ESP_OK;
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    STRIP_CHECK(index < ws2812->strip_len, "index out of the maximum number of leds", err, ESP_ERR_INVALID_ARG);
    uint32_t start = index * 3;
    // In thr order of GRB
    ws2812->buffer[start + 0] = green & 0xFF;
    ws2812->buffer[start + 1] = red & 0xFF;
    ws2812->buffer[start + 2] = blue & 0xFF;

    ESP_LOGD(TAG, "end of %s", __func__);
    return ESP_OK;
err:
    return ret;
}

esp_err_t ws2812_refresh(led_strip_t *strip, uint32_t timeout_ms)
{
    ESP_LOGI(TAG, "%s", __func__);
    
    esp_err_t ret = ESP_OK;
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
   STRIP_CHECK(
    rmt_transmit(ws2812->rmt_channel,
                 ws2812->encoder,
                 ws2812->buffer,
                 ws2812->strip_len * 3,
                 NULL) == ESP_OK,
    "transmit failed", err, ESP_FAIL
);

ESP_ERROR_CHECK(rmt_tx_wait_all_done(ws2812->rmt_channel, portMAX_DELAY));
return ESP_OK;
err:
    return ret;
}

esp_err_t ws2812_clear(led_strip_t *strip, uint32_t timeout_ms)
{
    ESP_LOGI(TAG, "%s", __func__);
    
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    // Write zero to turn off all leds
    memset(ws2812->buffer, 0, ws2812->strip_len * 3);
    ESP_LOGD(TAG, "end of %s", __func__);
    return ws2812_refresh(strip, timeout_ms);
}

esp_err_t ws2812_del(led_strip_t *strip)
{
    ESP_LOGI(TAG, "%s", __func__);
    
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    free(ws2812);
    return ESP_OK;
}

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config)
{
    ESP_LOGI(TAG, "%s", __func__);
    
    led_strip_t *ret = NULL;
    STRIP_CHECK(config, "configuration can't be null", err, NULL);

    // 24 bits per led
    uint32_t ws2812_size = sizeof(ws2812_t) + config->max_leds * 3;
    ws2812_t *ws2812 = calloc(1, ws2812_size);
    STRIP_CHECK(ws2812, "request memory for ws2812 failed", err, NULL);
    ESP_LOGI(TAG, "request memory for ws2812 correct");

    //ws2812->rmt_channel = (rmt_channel_t)config->dev;
    ws2812->rmt_channel = (rmt_channel_handle_t)config->dev;
    ws2812->strip_len = config->max_leds;

    ws2812->parent.set_pixel = ws2812_set_pixel;
    ws2812->parent.refresh = ws2812_refresh;
    ws2812->parent.clear = ws2812_clear;
    ws2812->parent.del = ws2812_del;

    return &ws2812->parent;
err:
    return ret;
}

esp_err_t led_rgb_init(led_strip_t **strip)
{
    ESP_LOGI(TAG, "Initializing embedded WS2812");

    // Crear canal RMT (NUEVO)
    rmt_tx_channel_config_t tx_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = KALUGA_RGB_LED_PIN,
        .mem_block_symbols = 64,
        .resolution_hz = 10 * 1000 * 1000,
        .trans_queue_depth = 4,
    };

    rmt_channel_handle_t channel = NULL;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_config, &channel));
    ESP_ERROR_CHECK(rmt_enable(channel));

    // Crear encoder WS2812
    rmt_encoder_handle_t encoder = NULL;

    rmt_bytes_encoder_config_t encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 4,
            .level1 = 0,
            .duration1 = 8,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 8,
            .level1 = 0,
            .duration1 = 4,
        },
        .flags.msb_first = 1
    };

    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&encoder_config, &encoder));

    // Crear strip
    led_strip_config_t strip_config = {
        .max_leds = KALUGA_RGB_LED_NUMBER,
        .dev = (led_strip_dev_t)channel,
    };

    *strip = led_strip_new_rmt_ws2812(&strip_config);

    if (!(*strip)) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
        return ESP_FAIL;
    }

    // Guardar encoder dentro del struct
    ws2812_t *ws = __containerof(*strip, ws2812_t, parent);
    ws->encoder = encoder;

    ESP_ERROR_CHECK((*strip)->clear((*strip), 100));

    return ESP_OK;
}