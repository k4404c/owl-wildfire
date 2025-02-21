/**
 * @file MamaDuck.ino
 * @brief An improved MamaDuck implementation with better error handling and resource management
 */

#include <string>
#include <arduino-timer.h>
#include <CDP.h>
#include "FastLED.h"
#include "Zanshin_BME680.h"
#include "random_forest_10.h"
#include <CircularBuffer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_task_wdt.h>
#include <TinyGPS++.h>

// Include our new header files
#include "DuckConfig.h"
#include "DuckError.h"
#include "DuckSensor.h"

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
BME680_Class BME680;
SensorManager sensorManager;

// Function declarations
bool sendData(std::vector<byte> message, topics value);
void IRAM_ATTR resetModule();
bool getGPSData(char* buffer, size_t bufferSize);
std::vector<byte> stringToByteVector(const String& str);

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
        // Clear the struct at the start of each iteration
        memset(&sensorData, 0, sizeof(SensorData));
        sensorData.timestamp = millis();
        
        // Get BME680 data with mutex protection
        if (xSemaphoreTake(bmeMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            static int32_t temp, humidity, pressure, gas;
            
            if (BME680.getSensorData(temp, humidity, pressure, gas)) {
                sensorData.temp = temp / 100.0;
                sensorData.humidity = humidity / 1000.0;
                sensorData.pressure = pressure / 100.0;
                sensorData.gas = gas / 100.0;
                
                if (!sensorData.validate()) {
                    DuckErrorHandler::setError(DuckStatus::ERROR_SENSOR_READ);
                }
            }
            xSemaphoreGive(bmeMutex);
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
        Serial.printf("[MAMA] Temp Volatility: %.4f\n", sensorData.temp_volatility);
        Serial.printf("[MAMA] Humidity Volatility: %.4f\n", sensorData.humidity_volatility);
        Serial.printf("[MAMA] Pressure Volatility: %.4f\n", sensorData.pressure_volatility);
        Serial.printf("[MAMA] Temp Velocity: %.4f\n", sensorData.temp_velocity);
        Serial.printf("[MAMA] Humidity Velocity: %.4f\n", sensorData.humidity_velocity);
        Serial.printf("[MAMA] Pressure Velocity: %.4f\n", sensorData.pressure_velocity);
        Serial.println("[MAMA] ===================================\n");
        
        // Get GPS data and print debug info
        Serial.println("[MAMA] --- GPS ---");
        Serial.print("[MAMA] Latitude  : "); Serial.println(tgps.location.lat(), 5);
        Serial.print("[MAMA] Longitude : "); Serial.println(tgps.location.lng(), 4);
        Serial.print("[MAMA] Altitude  : "); Serial.print(tgps.altitude.feet() / 3.2808); Serial.println("M");
        Serial.print("[MAMA] Satellites: "); Serial.println(tgps.satellites.value());
        Serial.print("[MAMA] Time      : ");
        Serial.print(tgps.time.hour()); Serial.print(":"); 
        Serial.print(tgps.time.minute()); Serial.print(":"); 
        Serial.println(tgps.time.second());
        Serial.print("[MAMA] Speed     : "); Serial.println(tgps.speed.kmph());
        Serial.println("[MAMA] **********************");
        
        // Get GPS data
        sensorData.hasValidGPS = getGPSData(gpsBuffer, sizeof(gpsBuffer));
        sensorData.setGPSData(gpsBuffer);
        
        // Make ML prediction
        float features[] = {
            sensorData.scaled_temp,
            sensorData.scaled_humidity,
            sensorData.scaled_pressure,
            sensorData.temp_volatility,
            sensorData.humidity_volatility,
            sensorData.pressure_volatility,
            sensorData.temp_velocity,
            sensorData.humidity_velocity,
            sensorData.pressure_velocity
        };
        
        Eloquent::ML::Port::RandomForest forest;
        sensorData.prediction = forest.predict(features);
        
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
                     "Counter:%d FM:%d Temp:%.2f Hum:%.3f Press:%.2f Gas:%.2f Pred:%d %s",
                     counter,
                     freeMemory(),
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
                leds[0] = CRGB::Green;
            } else {
                Serial.println("[MAMA] Packet transmission failed");
                leds[0] = CRGB::Red;
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
    // Initialize LED
    FastLED.addLeds<LED_TYPE, DuckConfig::SystemConfig::LED_PIN, COLOR_ORDER>(
        leds, 
        DuckConfig::SystemConfig::NUM_LEDS
    ).setCorrection(TypicalSMD5050);
    FastLED.setBrightness(DuckConfig::SystemConfig::LED_BRIGHTNESS);
    leds[0] = CRGB::Cyan;
    FastLED.show();

    // Initialize Serial
    Serial.begin(SERIAL_SPEED);

    // Initialize Duck with error handling
    std::string deviceId("ASUMAMA1");
    std::array<byte, 8> devId;
    std::copy_n(deviceId.begin(), 8, devId.begin());
    
    if (!DuckErrorHandler::retry("Duck Setup", [&]() {
        return duck.setupWithDefaults(devId) == DUCK_ERR_NONE;
    })) {
        return;
    }

    // Initialize GPS
    GPS.begin(9600, SERIAL_8N1, 34, 12);

    // Initialize BME680 with retry
    if (!DuckErrorHandler::retry("BME680 Setup", []() {
        return BME680.begin(I2C_STANDARD_MODE);
    })) {
        return;
    }

    // Configure BME680
    BME680.setOversampling(TemperatureSensor, Oversample16);
    BME680.setOversampling(HumiditySensor, Oversample16);
    BME680.setOversampling(PressureSensor, Oversample16);
    BME680.setIIRFilter(IIR4);
    BME680.setGas(320, 150);

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
    leds[0] = CRGB::Gold;
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