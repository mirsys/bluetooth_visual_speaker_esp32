/*
 * key.c
 *
 *  Created on: 2018-05-31 14:07
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "esp_log.h"
#include "esp_sleep.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "os/core.h"
#include "user/audio.h"

#define TAG "key"

#ifdef CONFIG_ENABLE_SLEEP_KEY
static void key_sleep_handle(void)
{
    xEventGroupClearBits(user_event_group, KEY_SCAN_BIT);

    audio_play_file(3);
    xEventGroupWaitBits(
        user_event_group,
        AUDIO_IDLE_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    os_enter_sleep_mode();
}
#endif // CONFIG_ENABLE_SLEEP_KEY

static void key_task_handle(void *pvParameter)
{
#ifdef CONFIG_ENABLE_SLEEP_KEY
    uint16_t count[] = {0};
    uint8_t gpio_pin[] = {CONFIG_SLEEP_KEY_PIN};
    uint8_t gpio_val[] = {
#ifdef CONFIG_SLEEP_KEY_ACTIVE_LOW
        0
#else
        1
#endif
    };
    uint16_t gpio_hold[] = {CONFIG_SLEEP_KEY_HOLD_TIME};
    void (*key_handle[])(void) = {key_sleep_handle};

    portTickType xLastWakeTime;

    for (int i=0; i<sizeof(gpio_pin); i++) {
        gpio_set_direction(gpio_pin[i], GPIO_MODE_INPUT);
        if (gpio_val[i] == 0) {
            gpio_pulldown_dis(gpio_pin[i]);
            gpio_pullup_en(gpio_pin[i]);
        } else {
            gpio_pullup_dis(gpio_pin[i]);
            gpio_pulldown_en(gpio_pin[i]);
        }
    }

    xEventGroupSetBits(user_event_group, KEY_SCAN_BIT);

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            KEY_SCAN_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        xLastWakeTime = xTaskGetTickCount();

        for (int i=0; i<sizeof(gpio_pin); i++) {
            if (gpio_get_level(gpio_pin[i]) == gpio_val[i]) {
                if (++count[i] == gpio_hold[i] / 10) {
                    count[i] = 0;
                    key_handle[i]();
                }
            } else {
                count[i] = 0;
            }
        }

        vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_RATE_MS);
    }
#endif // CONFIG_ENABLE_SLEEP_KEY
}

void key_init(void)
{
   xTaskCreate(key_task_handle, "keyT", 2048, NULL, 5, NULL);
}