/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "hardware/i2c.h"

#include "tof.h"
#include "tcp_client.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

char ssid[] = MY_SSID_NAME;
char pass[] = MY_SSID_PASS;

int main() {
    int i;
    int iDistance;
    int model, revision;

    stdio_init_all();
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_ISRAEL)) return 1;
    cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 200, 1, 1, 10);
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 10000)) return 1;

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
    
    TCP_CLIENT_T* client = tcp_client_init();
    tcp_client_connect(client);


    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    i = tofGetModel(&model, &revision);
    sprintf(client->buffer, "Model ID - %d\nRevision ID - %d\n", model, revision);
    tcp_client_send(client);
    i = tofInit(0, 0x29, 0);
    
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);

    // After all is done (with success), blink indefinetely
    int iter = 0;
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
        sleep_ms(LED_DELAY_MS);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
        sleep_ms(LED_DELAY_MS);

        sprintf(client->buffer, "iteration: %d\n", iter++);
        tcp_client_send(client);

        iDistance = tofReadDistance();
        sprintf(client->buffer, "Distance = %dmm\n", iDistance);
        tcp_client_send(client);
    }
}
