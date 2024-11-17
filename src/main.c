#include "tof.h"
#include "tcp_logger.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 100
#endif

//////////////////
// Helper methods
//////////////////
int init();
int initWiFi();
int initI2C();
void setOnboardLED(bool status);

#define SEND_HTTP() {sendHTTPMessage("192.168.0.104", 9988, "/receive", buffer);}
// #define SEND_HTTP() {sendLogMessage("192.168.0.104", 9988, buffer);}
// #define SEND_HTTP() {printf(buffer);}

//////////////////

int main() {
    init();

    setOnboardLED(true);
    char buffer[2048] = {0};
    sprintf(buffer, "Initializing...\n");
    SEND_HTTP();
    
    int i, iDistance, model, revision;
    i = tofInit(0, 0x29, 1);
    sprintf(buffer, "After tofInit, i=%d\n", i);
    SEND_HTTP();

    i = tofGetModel(&model, &revision);
    sprintf(buffer, "After tofGetModel, i=%d\n\tModel ID - %d\n\tRevision ID - %d\n", i, model, revision);
    SEND_HTTP();

    while (true) {
        setOnboardLED(true);
        sleep_ms(LED_DELAY_MS);
        setOnboardLED(false);
        sleep_ms(LED_DELAY_MS);

        // tofInit(0, 0x29, 1);
        iDistance = tofReadDistance() - 30; // Offset 30mm..
        sprintf(buffer, "distance: %d[mm]\n", iDistance);
        SEND_HTTP();
        sleep_ms(1000);

    }
}


// --------------------------------------------------------
int init() {
    int ret;
    if (ret = initWiFi()) return ret;
    if (ret = initI2C()) return ret;
    return 0;
}

int initWiFi() {
    char ssid[] = MY_SSID_NAME;
    char pass[] = MY_SSID_PASS;

    stdio_init_all();
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_ISRAEL)) return 1;
    cyw43_pm_value(CYW43_NO_POWERSAVE_MODE, 200, 1, 1, 10);
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 10000)) return 1;

    return 0;
}

int initI2C() {
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
}

void setOnboardLED(bool status) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, status);
}