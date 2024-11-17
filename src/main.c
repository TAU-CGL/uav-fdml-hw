#include "tof.h"
#include "tcp_logger.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 500
#endif

//////////////////
// Helper methods
//////////////////
void init();
void setOnboardLED(bool status);

//////////////////

int main() {
    init();
    
    char buffer[2048] = {0}; int iter = 0;
    while (true) {
        setOnboardLED(true);
        sleep_ms(LED_DELAY_MS);

        sprintf(buffer, "iteration: %d\n", iter++);
        sendHTTPMessage("192.168.0.104", 9988, "/receive", buffer);

        setOnboardLED(false);
        sleep_ms(LED_DELAY_MS);
    }
}




// --------------------------------------------------------
void init() {
    char ssid[] = MY_SSID_NAME;
    char pass[] = MY_SSID_PASS;

    stdio_init_all();
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_ISRAEL)) return 1;
    cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 200, 1, 1, 10);
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 10000)) return 1;
}

void setOnboardLED(bool status) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, status);
}