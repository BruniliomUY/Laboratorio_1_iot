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
#include "esp_err.h"
#include "esp_rom_sys.h"

#include "led_strip.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"

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

#define KALUGA_RGB_LED_PIN GPIO_NUM_45
#define KALUGA_RGB_LED_NUMBER 1

typedef struct
{
    led_strip_t parent;
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t encoder;
    uint32_t strip_len;
    uint8_t buffer[0];
} ws2812_t;

static esp_err_t ws2812_set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    esp_err_t ret = ESP_OK;
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    STRIP_CHECK(index < ws2812->strip_len, "index out of the maximum number of leds", err, ESP_ERR_INVALID_ARG);

    uint32_t start = index * 3;
    // GRB order
    ws2812->buffer[start + 0] = green & 0xFF;
    ws2812->buffer[start + 1] = red & 0xFF;
    ws2812->buffer[start + 2] = blue & 0xFF;

    return ESP_OK;
err:
    return ret;
}

static esp_err_t ws2812_refresh(led_strip_t *strip, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);

    STRIP_CHECK(ws2812->rmt_channel != NULL, "rmt channel not initialized", err, ESP_ERR_INVALID_STATE);
    STRIP_CHECK(ws2812->encoder != NULL, "encoder not initialized", err, ESP_ERR_INVALID_STATE);

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    STRIP_CHECK(
        rmt_transmit(ws2812->rmt_channel,
                     ws2812->encoder,
                     ws2812->buffer,
                     ws2812->strip_len * 3,
                     &tx_config) == ESP_OK,
        "transmit failed", err, ESP_FAIL
    );

    STRIP_CHECK(
        rmt_tx_wait_all_done(ws2812->rmt_channel, timeout_ms) == ESP_OK,
        "wait tx done failed", err, ESP_FAIL
    );

    // Reset/latch time para WS2812, sin usar FreeRTOS
    esp_rom_delay_us(300);

    return ESP_OK;
err:
    return ret;
}

static esp_err_t ws2812_clear(led_strip_t *strip, uint32_t timeout_ms)
{
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    memset(ws2812->buffer, 0, ws2812->strip_len * 3);
    return ws2812_refresh(strip, timeout_ms);
}

static esp_err_t ws2812_del(led_strip_t *strip)
{
    ws2812_t *ws2812 = __containerof(strip, ws2812_t, parent);
    if (ws2812->encoder) {
        rmt_del_encoder(ws2812->encoder);
    }
    if (ws2812->rmt_channel) {
        rmt_disable(ws2812->rmt_channel);
        rmt_del_channel(ws2812->rmt_channel);
    }
    free(ws2812);
    return ESP_OK;
}

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config)
{
    led_strip_t *ret = NULL;
    STRIP_CHECK(config, "configuration can't be null", err, NULL);

    // 24 bits per led -> 3 bytes por LED
    uint32_t ws2812_size = sizeof(ws2812_t) + config->max_leds * 3;
    ws2812_t *ws2812 = calloc(1, ws2812_size);
    STRIP_CHECK(ws2812, "request memory for ws2812 failed", err, NULL);

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
    esp_err_t ret = ESP_OK;
    rmt_channel_handle_t channel = NULL;
    rmt_encoder_handle_t encoder = NULL;

    STRIP_CHECK(strip != NULL, "strip pointer is null", err, ESP_ERR_INVALID_ARG);

    rmt_tx_channel_config_t tx_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = KALUGA_RGB_LED_PIN,
        .mem_block_symbols = 64,
        .resolution_hz = 10 * 1000 * 1000,
        .trans_queue_depth = 4,
    };

    STRIP_CHECK(rmt_new_tx_channel(&tx_config, &channel) == ESP_OK, "new tx channel failed", err, ESP_FAIL);
    STRIP_CHECK(rmt_enable(channel) == ESP_OK, "rmt enable failed", err, ESP_FAIL);

    rmt_bytes_encoder_config_t encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 3,
            .level1 = 0,
            .duration1 = 9,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 9,
            .level1 = 0,
            .duration1 = 3,
        },
        .flags.msb_first = 1
    };

    STRIP_CHECK(rmt_new_bytes_encoder(&encoder_config, &encoder) == ESP_OK, "new bytes encoder failed", err, ESP_FAIL);

    led_strip_config_t strip_config = {
        .max_leds = KALUGA_RGB_LED_NUMBER,
        .dev = (led_strip_dev_t)channel,
    };

    *strip = led_strip_new_rmt_ws2812(&strip_config);
    STRIP_CHECK(*strip != NULL, "install WS2812 driver failed", err, ESP_FAIL);

    ws2812_t *ws = __containerof(*strip, ws2812_t, parent);
    ws->encoder = encoder;

    return (*strip)->clear((*strip), 100);

err:
    if (encoder) {
        rmt_del_encoder(encoder);
    }
    if (channel) {
        rmt_disable(channel);
        rmt_del_channel(channel);
    }
    return ret;
}
