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

#define STRIP_NODE		DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct led_rgb colors[] = {
	RGB(0x10, 0x00, 0x00), /* red */
	RGB(0x00, 0x10, 0x00), /* green */
	RGB(0x00, 0x00, 0x10), /* blue */
};

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart), "Console device is not ACM CDC UART device");
static const struct device *const uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static const struct device *const i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));
static const struct device *const tsl2591 = DEVICE_DT_GET(DT_NODELABEL(tsl2591));

int main(void) {
    int ret = 0;
    size_t color = 0;
    struct sensor_value value_all, value_light, value_ir;

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
            printk("sensor_sample_fetch error %d", ret);
            continue;
        }

        ret |= sensor_channel_get(tsl2591, SENSOR_CHAN_ALL, &value_all);
        ret |= sensor_channel_get(tsl2591, SENSOR_CHAN_LIGHT, &value_light);
        ret |= sensor_channel_get(tsl2591, SENSOR_CHAN_IR, &value_ir);
        if (ret) {
            printk("sensor_channel_get error %d", ret);
            continue;
        }

        printk("SENSOR_CHAN_ALL     %d,%d\n", value_all.val1, value_all.val2);
        printk("SENSOR_CHAN_LIGHT   %d,%d\n", value_light.val1, value_light.val2);
        printk("SENSOR_CHAN_IR      %d,%d\n", value_ir.val1, value_ir.val2);

        k_sleep(K_MSEC(1000));

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
