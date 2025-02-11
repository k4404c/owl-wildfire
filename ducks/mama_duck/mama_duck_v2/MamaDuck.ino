/**
 * @file MamaDuck.ino
 * @brief A MamaDuck that sends GPS coordinates and environmental data with ML predictions.
 */

#include <string>
#include <arduino-timer.h>
#include <CDP.h>
#include "FastLED.h"
#include "Zanshin_BME680.h"
#include "random_forest_10.h"
#include <CircularBuffer.h>

//using namespace duckutils;

// LED Configuration
#define LED_TYPE WS2812
#define DATA_PIN 4
#define NUM_LEDS 1
#define COLOR_ORDER GRB
#define BRIGHTNESS 128
CRGB leds[NUM_LEDS];

// Serial configuration
#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// GPS Configuration
#include <TinyGPS++.h>
TinyGPSPlus tgps;
HardwareSerial GPS(1);

// Constants
#define SENSOR_READ_INTERVAL 300000  // 5 minutes
#define BME_READ_INTERVAL 10000      // 10 seconds
#define HISTORY_WINDOW_SIZE 5
const uint32_t SERIAL_SPEED = 115200;

// Scaler parameters for ML model
struct ScalerConfig {
    const float MEAN_TEMP = 20.363434343434342;     
    const float STD_TEMP = 1.5304664645867185;   
    const float MEAN_HUMIDITY = 26.952676767676767; 
    const float STD_HUMIDITY = 1.7995277373074992; 
    const float MEAN_PRESS = 977.121616161616;  
    const float STD_PRESS = 0.20419196291382694;     
};

// Global variables
MamaDuck duck;
auto timer = timer_create_default();
int counter = 1;
bool setupOK = false;
BME680_Class BME680;
ScalerConfig scalerConfig;

// Circular buffers for environmental data history
CircularBuffer<float, HISTORY_WINDOW_SIZE> tempHistory;
CircularBuffer<float, HISTORY_WINDOW_SIZE> humidityHistory;
CircularBuffer<float, HISTORY_WINDOW_SIZE> pressureHistory;

// Track previous values
float prev_scaled_temp = 0.0;
float prev_scaled_humidity = 0.0;
float prev_scaled_pressure = 0.0;
unsigned long lastSensorTime = 0;

// Function declarations
bool sendData(std::vector<byte> message, topics value);
bool runSensor(void *);
bool runBmeSensor(void *);
bool runGPS(void *);
float altitude(const int32_t press, const float seaLevel = 1013.25);
float computeVolatility(CircularBuffer<float, HISTORY_WINDOW_SIZE>& history);
void updateHistory(CircularBuffer<float, HISTORY_WINDOW_SIZE>& history, float newValue);
String getGPSData();

// Implementation of altitude calculation
float altitude(const int32_t press, const float seaLevel) {
    return 44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));
}

// Helper function to compute volatility
float computeVolatility(CircularBuffer<float, HISTORY_WINDOW_SIZE>& history) {
    float mean = 0.0, stdDev = 0.0;
    
    // Calculate mean
    for(int i = 0; i < HISTORY_WINDOW_SIZE; i++) {
        mean += history[i];
    }
    mean /= HISTORY_WINDOW_SIZE;
    
    // Calculate standard deviation
    for(int i = 0; i < HISTORY_WINDOW_SIZE; i++) {
        stdDev += pow(history[i] - mean, 2);
    }
    return sqrt(stdDev / HISTORY_WINDOW_SIZE);
}
std::vector<byte> stringToByteVector(const String& str) {
    std::vector<byte> byteVec;
    byteVec.reserve(str.length());
    for (unsigned int i = 0; i < str.length(); ++i) {
        byteVec.push_back(static_cast<byte>(str[i]));
    }
    return byteVec;
}


void setup() {
    // Initialize LED
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
    FastLED.setBrightness(BRIGHTNESS);
    leds[0] = CRGB::Cyan;
    FastLED.show();

    // Initialize Duck
    std::string deviceId("ASUMAMA1");
    std::array<byte, 8> devId;
    std::copy_n(deviceId.begin(), 8, devId.begin());
    
    if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE) {
        Serial.println("[MAMA] Failed to setup MamaDuck");
        return;
    }

    // Initialize GPS
    GPS.begin(9600, SERIAL_8N1, 34, 12);

    // Initialize BME680
    Serial.print(F("Starting BME680 initialization...\n"));
    while (!BME680.begin(I2C_STANDARD_MODE)) {
        Serial.print(F("-  Unable to find BME680. Trying again in 5 seconds.\n"));
        delay(5000);
    }

    // Configure BME680
    BME680.setOversampling(TemperatureSensor, Oversample16);
    BME680.setOversampling(HumiditySensor, Oversample16);
    BME680.setOversampling(PressureSensor, Oversample16);
    BME680.setIIRFilter(IIR4);
    BME680.setGas(320, 150);

    // Initialize timers
    timer.every(SENSOR_READ_INTERVAL, runSensor);
    timer.every(BME_READ_INTERVAL, runBmeSensor);

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
    timer.tick();
    duck.run();
}

static void smartDelay(unsigned long ms) {
    unsigned long start = millis();
    do {
        while (GPS.available())
            tgps.encode(GPS.read());
    } while (millis() - start < ms);
}

String getGPSData() {
    smartDelay(5000);
    
    // Log GPS data
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

    // Format GPS data string
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Lat:%.5f Lng:%.4f Alt:%.2f",
             tgps.location.lat(),
             tgps.location.lng(),
             tgps.altitude.feet() / 3.2808);

    return String(buffer);
}

bool runSensor(void *) {
    String gpsData = getGPSData();
    
    char buffer[200];
    snprintf(buffer, sizeof(buffer), "Counter:%d FM:%d %s",
             counter,
             freeMemory(),
             gpsData.c_str());
             
    Serial.print("[MAMA] sensor data: ");
    Serial.println(buffer);

    bool result = sendData(stringToByteVector(String(buffer)), location);

    if (result) {
        Serial.println("[MAMA] runSensor ok.");
    } else {
        Serial.println("[MAMA] runSensor failed.");
    }
    return result;
}

bool runBmeSensor(void *) {
    static int32_t temp, humidity, pressure, gas;
    
    if (!BME680.getSensorData(temp, humidity, pressure, gas)) {
        Serial.println("[MAMA] Failed to read BME680 data");
        return false;
    }

    // Scale the raw sensor values
    float scaled_temp = (temp / 100.0 - scalerConfig.MEAN_TEMP) / scalerConfig.STD_TEMP;
    float scaled_humidity = (humidity / 1000.0 - scalerConfig.MEAN_HUMIDITY) / scalerConfig.STD_HUMIDITY;
    float scaled_pressure = (pressure / 100.0 - scalerConfig.MEAN_PRESS) / scalerConfig.STD_PRESS;

    // Update histories
    tempHistory.push(scaled_temp);
    humidityHistory.push(scaled_humidity);
    pressureHistory.push(scaled_pressure);

    // Compute time interval for velocity
    unsigned long currentTime = millis();
    float timeIntervalHours = (currentTime - lastSensorTime) / 3600000.0;
    
    // Compute velocities
    float temp_velocity = lastSensorTime > 0 ? (scaled_temp - prev_scaled_temp) / timeIntervalHours : 0;
    float humidity_velocity = lastSensorTime > 0 ? (scaled_humidity - prev_scaled_humidity) / timeIntervalHours : 0;
    float pressure_velocity = lastSensorTime > 0 ? (scaled_pressure - prev_scaled_pressure) / timeIntervalHours : 0;

    // Update tracking variables
    lastSensorTime = currentTime;
    prev_scaled_temp = scaled_temp;
    prev_scaled_humidity = scaled_humidity;
    prev_scaled_pressure = scaled_pressure;

    // Compute volatilities
    float temp_volatility = computeVolatility(tempHistory);
    float humidity_volatility = computeVolatility(humidityHistory);
    float pressure_volatility = computeVolatility(pressureHistory);

    // Prepare features for prediction
    float features[] = {
        scaled_temp,
        scaled_humidity,
        scaled_pressure,
        temp_volatility,
        humidity_volatility,
        pressure_volatility,
        temp_velocity,
        humidity_velocity,
        pressure_velocity
    };

    // Make prediction
    Eloquent::ML::Port::RandomForest forest;
    int prediction = forest.predict(features);

    // Format message
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "Temp: %.2f, Humidity: %.3f, Pressure: %.2f, Alt: %.2f, Gas: %.2f, Prediction: %d",
             temp / 100.0,
             humidity / 1000.0,
             pressure / 100.0,
             altitude(pressure),
             gas / 100.0,
             prediction);

    Serial.print("[MAMA] sensor data: ");
    Serial.println(buffer);

   // print gps data
    String gpsData = getGPSData();
    snprintf(buffer, sizeof(buffer), "Counter:%d FM:%d %s",
             counter,
             freeMemory(),
             gpsData.c_str());

    bool result = sendData(stringToByteVector(String(buffer)), location);

    if (result) {
        Serial.println("[MAMA] runBMESensor ok.");
    } else {
        Serial.println("[MAMA] runBMESensor failed.");
    }

    return result;
}

bool sendData(std::vector<byte> message, topics value) {
    bool sentOk = false;
    
    int err = duck.sendData(value, message);
    if (err == DUCK_ERR_NONE) {
        counter++;
        sentOk = true;
    }
    if (!sentOk) {
        Serial.println("[MAMA] Failed to send data. error = " + String(err));
    }
    return sentOk;
}