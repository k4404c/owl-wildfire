/**
 * @file MamaDuck.ino
 * @brief An improved MamaDuck implementation with BME688 support using Bosch library
 */

#include <string>
#include <arduino-timer.h>
#include <CDP.h>
#include "FastLED.h"
#include <bme68x.h>
#include <bme68x_defs.h>
#include <Wire.h>
#include "random_forest_10_v2.h"
#include <CircularBuffer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_task_wdt.h>
#include <TinyGPS++.h>

// new header files
#include "DuckConfig.h"
#include "DuckError.h"
#include "DuckSensor.h"

// BME688 Configuration
struct bme68x_dev bme;
struct bme68x_conf conf;
struct bme68x_heatr_conf heatr_conf;
struct bme68x_data data;

// I2C Configuration
#define I2C_SDA 21  // may need to change these pins
#define I2C_SCL 22

// LED Configuration
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
CRGB leds[DuckConfig::SystemConfig::NUM_LEDS];

// Serial configuration
#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// GPS Configuration
TinyGPSPlus tgps;
HardwareSerial GPS(1);

// Constants
const uint32_t SERIAL_SPEED = 115200;

// Queue handles
QueueHandle_t sensorQueue;
QueueHandle_t transmitQueue;

// Mutex handles
SemaphoreHandle_t bmeMutex;
SemaphoreHandle_t gpsMutex;

// Task handles
TaskHandle_t mlProcessingTask;
TaskHandle_t packetTransmissionTask;

// Global variables
MamaDuck duck;
auto timer = timer_create_default();
int counter = 1;
bool setupOK = false;
SensorManager sensorManager;

// Function declarations
bool sendData(std::vector<byte> message, topics value);
void IRAM_ATTR resetModule();
bool getGPSData(char* buffer, size_t bufferSize);
std::vector<byte> stringToByteVector(const String& str);

// BME688 helper functions
BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    Wire.beginTransmission(BME68X_I2C_ADDR_LOW);
    Wire.write(reg_addr);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)BME68X_I2C_ADDR_LOW, (uint8_t)len);
    for (uint32_t i = 0; i < len; i++) {
        reg_data[i] = Wire.read();
    }
    return 0;
}

BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    Wire.beginTransmission(BME68X_I2C_ADDR_LOW);
    Wire.write(reg_addr);
    for (uint32_t i = 0; i < len; i++) {
        Wire.write(reg_data[i]);
    }
    Wire.endTransmission();
    return 0;
}

void bme68x_delay_us(uint32_t period, void *intf_ptr) {
    delayMicroseconds(period);
}

bool initBME688() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Initialize BME688
    bme.read = bme68x_i2c_read;
    bme.write = bme68x_i2c_write;
    bme.intf = BME68X_I2C_INTF;
    bme.delay_us = bme68x_delay_us;
    bme.amb_temp = 25;
    
    int8_t rslt = bme68x_init(&bme);
    if (rslt != BME68X_OK) return false;
    
    // Configure BME688
    conf.filter = BME68X_FILTER_SIZE_3;
    conf.odr = BME68X_ODR_NONE;
    conf.os_hum = BME68X_OS_16X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_16X;
    rslt = bme68x_set_conf(&conf, &bme);
    if (rslt != BME68X_OK) return false;
    
    // Perform burn-in procedure
    if (!performBME688Burnin()) return false;
    
    // Configure normal operation heater settings
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp = 320;
    heatr_conf.heatr_dur = 150;
    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);
    if (rslt != BME68X_OK) return false;
    
    return true;
}
bool performBME688Burnin() {
    // Configure heater for burn-in
    struct bme68x_heatr_conf burnin_conf;
    burnin_conf.enable = BME68X_ENABLE;
    burnin_conf.heatr_temp = 350; // Higher temperature for burn-in
    burnin_conf.heatr_dur = 300;  // Longer duration for burn-in
    
    int8_t rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &burnin_conf, &bme);
    if (rslt != BME68X_OK) return false;
    
    Serial.println("[MAMA] Starting BME688 burn-in procedure...");
    Serial.println("[MAMA] This will take approximately 5 minutes");
    
    // Perform burn-in cycles
    const int BURN_IN_CYCLES = 40;
    float sum_gas = 0;
    int valid_readings = 0;
    
    for (int i = 0; i < BURN_IN_CYCLES; i++) {
        // Set to forced mode
        rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
        if (rslt != BME68X_OK) continue;
        
        // Get the delay period required between measurements
        uint32_t del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) 
                             + (burnin_conf.heatr_dur * 1000);
        bme.delay_us(del_period, NULL);
        
        // Get data
        uint8_t n_fields;
        struct bme68x_data data;
        rslt = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme);
        
        if (rslt == BME68X_OK && n_fields && 
            (data.status & BME68X_GASM_VALID_MSK) && 
            (data.status & BME68X_HEAT_STAB_MSK)) {
            sum_gas += data.gas_resistance;
            valid_readings++;
            
            Serial.printf("[MAMA] Burn-in cycle %d/%d - Gas resistance: %.2f kOhms\n", 
                         i + 1, BURN_IN_CYCLES, data.gas_resistance / 1000.0);
        }
        
        delay(2000); // Wait between measurements
        
        // Update LED to show progress
        leds[0] = CRGB(0, (i * 255) / BURN_IN_CYCLES, 0);
        FastLED.show();
    }
    
    // Calculate baseline if we got enough valid readings
    if (valid_readings > BURN_IN_CYCLES / 2) {
        float baseline = sum_gas / valid_readings;
        Serial.printf("[MAMA] Burn-in complete. Baseline gas resistance: %.2f kOhms\n", 
                     baseline / 1000.0);
        // You could store this baseline value in non-volatile memory here
        return true;
    }
    
    Serial.println("[MAMA] Burn-in failed - not enough valid readings");
    return false;
}

bool getBME688Data(float &temp, float &humidity, float &pressure, float &gas) {
    if (xSemaphoreTake(bmeMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }
    
    int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
    if (rslt != BME68X_OK) {
        xSemaphoreGive(bmeMutex);
        return false;
    }
    
    uint32_t del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);
    bme.delay_us(del_period, NULL);
    
    uint8_t n_fields;
    rslt = bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &bme);
    
    if (rslt == BME68X_OK && n_fields) {
        temp = data.temperature;
        pressure = data.pressure;
        humidity = data.humidity;
        gas = data.gas_resistance;
        xSemaphoreGive(bmeMutex);
        return true;
    }
    
    xSemaphoreGive(bmeMutex);
    return false;
}

// Implementation of utility functions
void IRAM_ATTR resetModule() {
    esp_restart();
}

bool getGPSData(char* buffer, size_t bufferSize) {
    if (xSemaphoreTake(gpsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }

    unsigned long startTime = millis();
    bool gotFix = false;

    while (millis() - startTime < DuckConfig::SystemConfig::GPS_TIMEOUT) {
        while (GPS.available()) {
            if (tgps.encode(GPS.read())) {
                if (tgps.location.isValid()) {
                    snprintf(buffer, bufferSize,
                            "Lat:%.5f Lng:%.4f Alt:%.2f",
                            tgps.location.lat(),
                            tgps.location.lng(),
                            tgps.altitude.feet() / 3.2808);
                    gotFix = true;
                    break;
                }
            }
        }
        if (gotFix) break;
        delay(10);
    }

    xSemaphoreGive(gpsMutex);

    if (!gotFix) {
        DuckErrorHandler::setError(DuckStatus::WARNING_GPS_NO_FIX);
        strncpy(buffer, "NO_FIX", bufferSize);
        return false;
    }

    return true;
}

std::vector<byte> stringToByteVector(const String& str) {
    std::vector<byte> byteVec;
    byteVec.reserve(str.length());
    for (unsigned int i = 0; i < str.length(); ++i) {
        byteVec.push_back(static_cast<byte>(str[i]));
    }
    return byteVec;
}

bool sendData(std::vector<byte> message, topics value) {
    return DuckErrorHandler::retry("Send Data", [&]() {
        int err = duck.sendData(value, message);
        return err == DUCK_ERR_NONE;
    });
}

// Task to handle ML processing
void mlProcessingLoop(void* parameter) {
    SensorData sensorData;
    static char gpsBuffer[DuckConfig::SystemConfig::GPS_BUFFER_SIZE];
    
    while (true) {
        memset(&sensorData, 0, sizeof(SensorData));
        sensorData.timestamp = millis();
        
        // Get BME688 data
        float temp, humidity, pressure, gas;
        if (getBME688Data(temp, humidity, pressure, gas)) {
            sensorData.temp = temp;
            sensorData.humidity = humidity;
            sensorData.pressure = pressure;
            sensorData.gas = gas;
            
            if (!sensorData.validate()) {
                DuckErrorHandler::setError(DuckStatus::ERROR_SENSOR_READ);
            }
        }
        
        // Print sensor data report
        Serial.println("\n[MAMA] ======== SENSOR DATA REPORT ========");
        Serial.println("[MAMA] ----- Environmental Readings -----");
        Serial.printf("[MAMA] Temperature: %.2f°C\n", sensorData.temp);
        Serial.printf("[MAMA] Humidity: %.3f%%\n", sensorData.humidity);
        Serial.printf("[MAMA] Pressure: %.2f hPa\n", sensorData.pressure);
        Serial.printf("[MAMA] Gas: %.2f\n", sensorData.gas);
        
        // Process sensor data
        sensorManager.processSensorData(sensorData);
        
        // Print processed data
        Serial.println("[MAMA] ----- ML Feature Processing -----");
        Serial.printf("[MAMA] Scaled Temp: %.4f\n", sensorData.scaled_temp);
        Serial.printf("[MAMA] Scaled Humidity: %.4f\n", sensorData.scaled_humidity);
        Serial.printf("[MAMA] Scaled Pressure: %.4f\n", sensorData.scaled_pressure);
        Serial.printf("[MAMA] Scaled Gas: %.4f\n", sensorData.scaled_gas);
        Serial.printf("[MAMA] Temp Volatility: %.4f\n", sensorData.temp_volatility);
        Serial.printf("[MAMA] Humidity Volatility: %.4f\n", sensorData.humidity_volatility);
        Serial.printf("[MAMA] Pressure Volatility: %.4f\n", sensorData.pressure_volatility);
        // add gas volatility
        Serial.printf("[MAMA] Temp Velocity: %.4f\n", sensorData.temp_velocity);
        Serial.printf("[MAMA] Humidity Velocity: %.4f\n", sensorData.humidity_velocity);
        Serial.printf("[MAMA] Pressure Velocity: %.4f\n", sensorData.pressure_velocity);
        Serial.printf("[MAMA] Gas Velocity: %.4f\n", sensorData.gas_velocity);
        //Serial.println("[MAMA] ===================================\n");
        
        // Get GPS data and print debug info
        Serial.println("[MAMA] --------- GPS ---------");
        Serial.print("[MAMA] Latitude  : "); Serial.println(tgps.location.lat(), 5);
        Serial.print("[MAMA] Longitude : "); Serial.println(tgps.location.lng(), 4);
        Serial.print("[MAMA] Altitude  : "); Serial.print(tgps.altitude.feet() / 3.2808); Serial.println("M");
        Serial.print("[MAMA] Satellites: "); Serial.println(tgps.satellites.value());
        Serial.print("[MAMA] Time      : ");
        Serial.print(tgps.time.hour()); Serial.print(":"); 
        Serial.print(tgps.time.minute()); Serial.print(":"); 
        Serial.println(tgps.time.second());
        Serial.print("[MAMA] Speed     : "); Serial.println(tgps.speed.kmph());
        //Serial.println("[MAMA] **********************");
        
        // Get GPS data
        sensorData.hasValidGPS = getGPSData(gpsBuffer, sizeof(gpsBuffer));
        sensorData.setGPSData(gpsBuffer);
        
        // Make ML prediction
        float features[] = {
            sensorData.scaled_temp,
            sensorData.scaled_humidity,
            sensorData.scaled_pressure,
            sensorData.scaled_gas,
            sensorData.temp_volatility,
            sensorData.humidity_volatility,
            sensorData.pressure_volatility,
            sensorData.gas_volatility,
            sensorData.temp_velocity,
            sensorData.humidity_velocity,
            sensorData.pressure_velocity,
            sensorData.gas_velocity
        };
        
        Eloquent::ML::Port::RandomForest forest;
        sensorData.prediction = forest.predict(features);
        Serial.println("[MAMA] ----- ML Prediction -----");
        Serial.printf("[MAMA] Prediction: %d\n", sensorData.prediction);
        Serial.println("[MAMA] ===================================\n");
        
        // Send to transmission queue with timeout
        if (xQueueSend(transmitQueue, &sensorData, pdMS_TO_TICKS(1000)) != pdTRUE) {
            DuckErrorHandler::setError(DuckStatus::ERROR_QUEUE_FULL);
        }
        
        // Check stack health
        DuckErrorHandler::checkStackHealth(
            mlProcessingTask, 
            "ML_Processing", 
            1024
        );
        
        vTaskDelay(pdMS_TO_TICKS(DuckConfig::SystemConfig::BME_READ_INTERVAL));
    }
}

// Task to handle packet transmission
void packetTransmissionLoop(void* parameter) {
    SensorData sensorData;
    static char messageBuffer[DuckConfig::SystemConfig::MESSAGE_BUFFER_SIZE];
    
    while (true) {
        if (xQueueReceive(transmitQueue, &sensorData, pdMS_TO_TICKS(1000)) == pdTRUE) {
            snprintf(messageBuffer, sizeof(messageBuffer),
                     "Counter:%d Temp:%.2f Hum:%.3f Press:%.2f Gas:%.2f Pred:%d %s",
                     counter,
                     sensorData.temp,
                     sensorData.humidity,
                     sensorData.pressure,
                     sensorData.gas,
                     sensorData.prediction,
                     sensorData.gpsData);
            
            bool result = sendData(stringToByteVector(String(messageBuffer)), location);
            
            if (result) {
                Serial.println("[MAMA] Packet transmission successful");
                counter++;
                leds[0] = CRGB::Green; // Green indicates successful transmission
            } else {
                Serial.println("[MAMA] Packet transmission failed");
                leds[0] = CRGB::Red; // Red indicates failed transmission
            }
            FastLED.show();
        }
        
        // Check stack health
        DuckErrorHandler::checkStackHealth(
            packetTransmissionTask, 
            "Packet_Transmission", 
            1024
        );
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup() {
    // Initialize Serial
    Serial.begin(SERIAL_SPEED);
    delay(1000); // Wait for Serial to initialize

    // Initialize LED
    FastLED.addLeds<LED_TYPE, DuckConfig::SystemConfig::LED_PIN, COLOR_ORDER>(
        leds, 
        DuckConfig::SystemConfig::NUM_LEDS
    ).setCorrection(TypicalSMD5050);
    FastLED.setBrightness(DuckConfig::SystemConfig::LED_BRIGHTNESS);
    leds[0] = CRGB::Cyan; // Cyan indicates setup in progress
    FastLED.show();

    // Initialize Duck with error handling
    std::string deviceId("ASUMAMA2"); //CHANGE WHEN FLASHING TO MULTIPLE DUCKS
    std::array<byte, 8> devId;
    std::copy_n(deviceId.begin(), 8, devId.begin());
    
    if (!DuckErrorHandler::retry("Duck Setup", [&]() {
        return duck.setupWithDefaults(devId) == DUCK_ERR_NONE;
    })) {
        return;
    }

    // Initialize GPS
    GPS.begin(9600, SERIAL_8N1, 34, 12);

    // Initialize BME688 with retry
    if (!DuckErrorHandler::retry("BME688 Setup", []() {
        return initBME688();
    })) {
        return;
    }

    // Create mutexes
    bmeMutex = xSemaphoreCreateMutex();
    gpsMutex = xSemaphoreCreateMutex();
    
    if (!bmeMutex || !gpsMutex) {
        DuckErrorHandler::setError(DuckStatus::ERROR_SENSOR_READ, "Failed to create mutexes");
        return;
    }
    
    // Create queues
    sensorQueue = xQueueCreate(5, sizeof(SensorData));
    transmitQueue = xQueueCreate(5, sizeof(SensorData));
    
    if (!sensorQueue || !transmitQueue) {
        DuckErrorHandler::setError(DuckStatus::ERROR_QUEUE_FULL, "Failed to create queues");
        return;
    }
    
    // Create tasks
    BaseType_t mlTaskCreated = xTaskCreatePinnedToCore(
        mlProcessingLoop,
        "ML_Processing",
        DuckConfig::SystemConfig::ML_STACK_SIZE,
        NULL,
        2,
        &mlProcessingTask,
        0
    );
    
    if (mlTaskCreated != pdPASS) {
        DuckErrorHandler::setError(DuckStatus::ERROR_SENSOR_READ, "Failed to create ML task");
        return;
    }
    
    BaseType_t txTaskCreated = xTaskCreatePinnedToCore(
        packetTransmissionLoop,
        "Packet_Transmission",
        DuckConfig::SystemConfig::TX_STACK_SIZE,
        NULL,
        1,
        &packetTransmissionTask,
        1
    );
    
    if (txTaskCreated != pdPASS) {
        DuckErrorHandler::setError(DuckStatus::ERROR_TRANSMISSION, "Failed to create TX task");
        return;
    }

    // Configure watchdog
    esp_task_wdt_init(DuckConfig::SystemConfig::WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);
    
    // Setup complete
    leds[0] = CRGB::Gold; // Gold indicates setup complete
    FastLED.show();
    setupOK = true;
    Serial.println("[MAMA] Setup OK!");
}

void loop() {
    if (!setupOK) {
        return;
    }
    duck.run();
    esp_task_wdt_reset();
}