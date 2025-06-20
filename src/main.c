/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/util.h>
#include <zephyr/data/json.h>

#define STRIP_NODE		DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart), "Console device is not ACM CDC UART device");
static const struct device *const uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static const struct device *const tsl2591 = DEVICE_DT_GET(DT_NODELABEL(tsl2591));

int main(void) {
    int ret = 0;
    struct led_rgb color = {.r = 0x00, .g = 0x00, .b = 0x00};
    struct sensor_value value_all, value_light, value_ir;
    int32_t lux;
    uint16_t output;

    struct json_data {
        int32_t lux;
        int32_t visible;
        int32_t infrared;
        //uint16_t output;
    } json;
    static const struct json_obj_descr descr[] = {
        JSON_OBJ_DESCR_PRIM(struct json_data, lux, JSON_TOK_NUMBER),
        JSON_OBJ_DESCR_PRIM(struct json_data, visible, JSON_TOK_NUMBER),
        JSON_OBJ_DESCR_PRIM(struct json_data, infrared, JSON_TOK_NUMBER),
        //JSON_OBJ_DESCR_PRIM(struct json_data, output, JSON_TOK_NUMBER),
    };


    if (!device_is_ready(uart))
        return 0;

    if (usb_enable(NULL))
        return 0;

    if (!device_is_ready(strip))
        return 0;

    if (!device_is_ready(tsl2591))
        return 0;

    printk("%s\n", CONFIG_BOARD_TARGET);

    while (1) {

        /* Unfortunately, datasheet does not provide a lux conversion formula for this particular
         * device. There is still ongoing discussion about the proper formula, though this
         * implementation uses a slightly modified version of the Adafruit library formula:
         * https://github.com/adafruit/Adafruit_TSL2591_Library/
         *
         * Since the device relies on both visible and IR readings to calculate lux,
         * read SENSOR_CHAN_ALL to get a closer approximation of lux. Reading SENSOR_CHAN_LIGHT or
         * SENSOR_CHAN_IR individually can be more closely thought of as relative strength
         * as opposed to true lux.
         */
        ret = sensor_sample_fetch(tsl2591);
        if (ret) {
            printk("sensor_sample_fetch %d", ret);
            continue;
        }

        ret |= sensor_channel_get(tsl2591, SENSOR_CHAN_ALL, &value_all);
        ret |= sensor_channel_get(tsl2591, SENSOR_CHAN_LIGHT, &value_light);
        ret |= sensor_channel_get(tsl2591, SENSOR_CHAN_IR, &value_ir);
        if (ret) {
            printk("sensor_channel_get %d", ret);
            continue;
        }
        
        color.g = 0x10;
        ret = led_strip_update_rgb(strip, &color, STRIP_NUM_PIXELS);
        if (ret) {
            LOG_ERR("led_strip_update_rgb %d", ret);
        }

        lux = value_all.val1;

        uint16_t lux_min = 0;
        uint16_t lux_max = 800;
        uint16_t lux_range = lux_max - lux_min;
        uint16_t output_min = 0;
        uint16_t output_max = 80;
        uint16_t output_range = output_max - output_min;
        output = (((lux - lux_min) * output_range) / lux_range) + output_min;

        /*
        printk("SENSOR_CHAN_ALL     %d,%d\n", value_all.val1, value_all.val2);
        printk("SENSOR_CHAN_LIGHT   %d,%d\n", value_light.val1, value_light.val2);
        printk("SENSOR_CHAN_IR      %d,%d\n", value_ir.val1, value_ir.val2);
        printk("lux, output         %d,%d\n\n", lux, output);
*/
        json.lux = lux;
        json.visible = value_light.val1;
        json.infrared = value_ir.val1;
        //json.output = output;
        char buf[256] = {0};
        ret = json_obj_encode_buf(descr, ARRAY_SIZE(descr), &json, buf, sizeof(buf));
        if (ret != 0)
            printk("json error %d\n", ret);
        else
            printk("%s\n", buf);

        k_sleep(K_MSEC(100));
        color.g = 0x00;
        ret = led_strip_update_rgb(strip, &color, STRIP_NUM_PIXELS);
        if (ret) {
            LOG_ERR("led_strip_update_rgb %d", ret);
        }

        k_sleep(K_MSEC(900));

        /*
		for (size_t cursor = 0; cursor < ARRAY_SIZE(pixels); cursor++) {
			memset(&pixels, 0x00, sizeof(pixels));
			memcpy(&pixels[cursor], &colors[color], sizeof(struct led_rgb));

			ret = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
			if (ret) {
				LOG_ERR("couldn't update strip: %d", ret);
			}

			k_sleep(K_MSEC(5000));
		}

		color = (color + 1) % ARRAY_SIZE(colors);
        */
	}

	return 0;
}
