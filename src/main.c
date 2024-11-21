#include "tcp_logger.h"

#include "VL53L1X_api.h"
#include "VL53L1X_types.h"
#define I2C_DEV_ADDR 0x29

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 100
#endif

#define DATABASE_RESET_KEYWORD "$$$RESET_DB$$$"

//////////////////
// Helper methods
//////////////////
int init();
int initWiFi();
int initI2C();
void setOnboardLED(bool status);
void blinkLED();

void setActiveSensor(int idx);
int initSingleSensor(int idx);
uint16_t sampleSingleDistance(int idx);

#define BUFFER_SIZE 2048
#define SEND_HTTP() {sendHTTPMessage("192.168.0.104", 9988, "/receive", buffer);}
// #define SEND_HTTP() {sendLogMessage("192.168.0.104", 9988, buffer);}
// #define SEND_HTTP() {printf(buffer);}

#define NUM_SENSORS 15
#define A0_PIN 26
#define A1_PIN 27
#define A2_PIN 28

// Helper variables
char buffer[BUFFER_SIZE] = {0};
uint8_t dataReady;
VL53L1X_Status_t status;
VL53L1X_Result_t results;
bool firstRange[NUM_SENSORS];
bool skipSensor[NUM_SENSORS];

//////////////////

int main() {
    init();

    setOnboardLED(true);

    sprintf(buffer, DATABASE_RESET_KEYWORD);
    SEND_HTTP();
    sprintf(buffer, "Initializing...\n");
    SEND_HTTP();

    for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
        firstRange[idx] = true;
        skipSensor[idx] = true;
    }
    // skipSensor[3] = false;
    // skipSensor[4] = false;
    skipSensor[5] = false; 

    for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
        if (skipSensor[idx]) continue;
        setActiveSensor(idx);
        initSingleSensor(idx);
    }
    
    while (true) {
        blinkLED();
        for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
            if (skipSensor[idx]) continue;
            setActiveSensor(idx);
            uint16_t distance = sampleSingleDistance(idx);
            sprintf(buffer, "{{%d}}\tdistance: %d[mm]\n", idx, distance);
            SEND_HTTP();
        }
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
    gpio_init(A0_PIN); gpio_set_dir(A0_PIN, GPIO_OUT);
    gpio_init(A1_PIN); gpio_set_dir(A1_PIN, GPIO_OUT);
    gpio_init(A2_PIN); gpio_set_dir(A2_PIN, GPIO_OUT);
}

void setOnboardLED(bool status) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, status);
}
void blinkLED() {
    setOnboardLED(true);
    sleep_ms(LED_DELAY_MS);
    setOnboardLED(false);
    sleep_ms(LED_DELAY_MS);
}

int initSingleSensor(int idx) {
    uint8_t sensorState;

    sprintf(buffer, "Initializing sensor %d...\n", idx);
    SEND_HTTP();
    
    if (VL53L1X_I2C_Init(I2C_DEV_ADDR, i2c0) < 0) {
        sprintf(buffer, "Error initializing sensor %d.\n", idx);
        SEND_HTTP();
        return 0;
    }

    // Ensure the sensor has booted
    do {
        status += VL53L1X_BootState(I2C_DEV_ADDR, &sensorState);
        VL53L1X_WaitMs(I2C_DEV_ADDR, 2);
    } while(!sensorState);
    sprintf(buffer, "Sensor %d booted.\n", idx);
    SEND_HTTP();

    status = VL53L1X_SensorInit(I2C_DEV_ADDR);
    status += VL53L1X_SetDistanceMode(I2C_DEV_ADDR, 1);
    status += VL53L1X_SetTimingBudgetInMs(I2C_DEV_ADDR, 100);
    status += VL53L1X_SetInterMeasurementInMs(I2C_DEV_ADDR, 100);
    status += VL53L1X_StartRanging(I2C_DEV_ADDR);
}
uint16_t sampleSingleDistance(int idx){
    do {
        status = VL53L1X_CheckForDataReady(I2C_DEV_ADDR, &dataReady);
        sleep_us(1);
    } while (!dataReady);

    status += VL53L1X_GetResult(I2C_DEV_ADDR, &results);
    status += VL53L1X_ClearInterrupt(I2C_DEV_ADDR);
    if (firstRange[idx]) {
        status += VL53L1X_ClearInterrupt(I2C_DEV_ADDR);
        firstRange[idx] = false;
    }
    return results.distance;
}
void setActiveSensor(int idx) {
    gpio_put(A0_PIN, idx & 0x01);
    gpio_put(A1_PIN, idx & 0x02 >> 1);
    gpio_put(A2_PIN, idx & 0x04 >> 2);
    sleep_ms(100);
    sprintf(buffer, "Set active sensor %d...\n", idx);
    SEND_HTTP();
}