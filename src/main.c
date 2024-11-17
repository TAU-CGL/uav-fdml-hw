/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tcp_logger.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

char ssid[] = MY_SSID_NAME;
char pass[] = MY_SSID_PASS;

int main() {
    stdio_init_all();
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_ISRAEL)) return 1;
    cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 200, 1, 1, 10);
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 10000)) return 1;

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
    
    char buffer[2048] = {0};

    printf("-------------------------------------------------------\n");

    // After all is done (with success), blink indefinetely
    int iter = 0;
    int num_fails = 0;
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        sleep_ms(LED_DELAY_MS);

        sprintf(buffer, "iteration: %d\tfailes: %d\n", iter++, num_fails);
        num_fails += !sendLogMessage("192.168.0.104", 9988, buffer);

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
        sleep_ms(LED_DELAY_MS);
    }
}
