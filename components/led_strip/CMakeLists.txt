set(component_srcs "src/led_strip_rmt_ws2812.c")

idf_component_register(
    SRCS "src/led_strip_rmt_ws2812.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_rmt driver
)

