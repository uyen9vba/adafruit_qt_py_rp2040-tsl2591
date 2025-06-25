/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/data/json.h>

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart), "Where is ACM CDC UART device");

static const struct device *const strip = DEVICE_DT_GET(DT_ALIAS(led_strip));
static const struct device *const uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static const struct device *const tsl2591 = DEVICE_DT_GET(DT_NODELABEL(tsl2591));

int main(void) {
    int ret = 0;
    struct led_rgb color = {.r = 0x00, .g = 0x00, .b = 0x00};
    struct sensor_value value_all, value_light, value_ir;
    struct json_data {
        int32_t lux;
        int32_t visible;
        int32_t infrared;
    } json;
    static const struct json_obj_descr descr[] = {
        JSON_OBJ_DESCR_PRIM(struct json_data, lux, JSON_TOK_NUMBER),
        JSON_OBJ_DESCR_PRIM(struct json_data, visible, JSON_TOK_NUMBER),
        JSON_OBJ_DESCR_PRIM(struct json_data, infrared, JSON_TOK_NUMBER),
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

    int64_t stamp = k_uptime_get();

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

        json.lux = value_all.val1;
        json.visible = value_light.val1;
        json.infrared = value_ir.val1;
        char buf[256] = {0};
        ret = json_obj_encode_buf(descr, ARRAY_SIZE(descr), &json, buf, sizeof(buf));
        if (ret != 0)
            printk("json error %d\n", ret);
        else
            printk("%s\n", buf);

        /*
         * Flash LED periodically to indicate the no errors have occurred.
         */
        int64_t delta = k_uptime_get() - stamp;
        if (delta > 1000)
            stamp = k_uptime_get();
        if (delta <= 900)
            color.g = 0x00;
        else if (delta <= 1000)
            color.g = 0x10;
        ret = led_strip_update_rgb(strip, &color, STRIP_NUM_PIXELS);
        if (ret)
            printk("led_strip_update_rgb %d", ret);

        k_sleep(K_MSEC(1));
	}

	return 0;
}
