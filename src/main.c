#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "tcp_logger.h"

#include "VL53L1X_api.h"
#include "VL53L1X_types.h"

#define I2C_DEV_ADDR 0x29
#define I2C_MUX0_ADDR 0x70
#define I2C_MUX1_ADDR 0x71

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

void setActiveSensor(uint8_t idx, bool verbose);
int initSingleSensor(uint8_t idx);
uint16_t sampleSingleDistance(uint8_t idx);

#define BUFFER_SIZE 2048
#define SEND_HTTP() {sendHTTPMessage("192.168.0.104", 9988, "/receive", buffer);}
// #define SEND_HTTP() {sendLogMessage("192.168.0.104", 9988, buffer);}
// #define SEND_HTTP() {printf(buffer);}

#define NUM_SENSORS 16

// Helper variables
char buffer[BUFFER_SIZE] = {0};
uint8_t dataReady;
VL53L1X_Status_t status;
VL53L1X_Result_t results;
bool firstRange[NUM_SENSORS];
bool skipSensor[NUM_SENSORS];
uint16_t distances[NUM_SENSORS];

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
    skipSensor[3+8] = false;
    skipSensor[4+8] = false;
    skipSensor[5+8] = false; 

    for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
        if (skipSensor[idx]) continue;
        setActiveSensor(idx, true);
        initSingleSensor(idx);
    }
    
    while (true) {
        blinkLED();
        for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
            distances[idx] = 0;
            if (skipSensor[idx]) continue;
            setActiveSensor(idx, false);
            distances[idx] = sampleSingleDistance(idx);
        }
        // Convert distances to string (in buffer)
        sprintf(buffer, "Distances[mm]: ");
        for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
            sprintf(buffer + strlen(buffer), "%d, ", distances[idx]);
        }
        buffer[strlen(buffer)-2] = '\0';
        SEND_HTTP();
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
    i2c_init(i2c_default, VL53L1X_I2C_BAUDRATE);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
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

int initSingleSensor(uint8_t idx) {
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
uint16_t sampleSingleDistance(uint8_t idx){
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
void setActiveSensor(uint8_t idx, bool verbose) {
    if (verbose) {
        sprintf(buffer, "Set active sensor %d...\n", idx);
        SEND_HTTP();
    }
    
    uint8_t addr = I2C_MUX0_ADDR;
    if (idx >= 0x8) {
        addr = I2C_MUX1_ADDR;
        idx -= 0x8;
    }
    uint8_t data = 1 << idx;
    if (i2c_write_blocking(i2c_default, addr, &data, 1, false) != 1) {
        sprintf(buffer, "Failed setting active sensor %d!\n", idx);
        SEND_HTTP();
    }
}