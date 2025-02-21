#ifndef DUCK_SENSOR_H
#define DUCK_SENSOR_H

#include <CircularBuffer.h>
#include "DuckConfig.h"
#include "DuckError.h"

struct SensorData {
    // Raw sensor readings
    float temp;
    float humidity;
    float pressure;
    float gas;
    
    // Processed features
    float scaled_temp;
    float scaled_humidity;
    float scaled_pressure;
    float temp_volatility;
    float humidity_volatility;
    float pressure_volatility;
    float temp_velocity;
    float humidity_velocity;
    float pressure_velocity;
    
    // ML prediction
    int prediction;
    
    // GPS data
    char gpsData[DuckConfig::SystemConfig::GPS_BUFFER_SIZE];
    bool hasValidGPS;
    
    // Timestamp
    unsigned long timestamp;

    void setGPSData(const char* data) {
        strncpy(gpsData, data, DuckConfig::SystemConfig::GPS_BUFFER_SIZE - 1);
        gpsData[DuckConfig::SystemConfig::GPS_BUFFER_SIZE - 1] = '\0';
    }

    bool validate() const {
        if (temp < DuckConfig::SensorBounds::MIN_TEMPERATURE || 
            temp > DuckConfig::SensorBounds::MAX_TEMPERATURE) {
            return false;
        }
        if (humidity < DuckConfig::SensorBounds::MIN_HUMIDITY || 
            humidity > DuckConfig::SensorBounds::MAX_HUMIDITY) {
            return false;
        }
        if (pressure < DuckConfig::SensorBounds::MIN_PRESSURE || 
            pressure > DuckConfig::SensorBounds::MAX_PRESSURE) {
            return false;
        }
        return true;
    }
};

class SensorManager {
private:
    CircularBuffer<float, DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE> tempHistory;
    CircularBuffer<float, DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE> humidityHistory;
    CircularBuffer<float, DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE> pressureHistory;
    
    float prev_scaled_temp = 0.0f;
    float prev_scaled_humidity = 0.0f;
    float prev_scaled_pressure = 0.0f;
    unsigned long lastSensorTime = 0;

    static float computeVolatility(const CircularBuffer<float, DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE>& history) {
        float mean = 0.0f, stdDev = 0.0f;
        
        // Calculate mean
        for(int i = 0; i < DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE; i++) {
            mean += history[i];
        }
        mean /= DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE;
        
        // Calculate standard deviation
        for(int i = 0; i < DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE; i++) {
            stdDev += pow(history[i] - mean, 2);
        }
        return sqrt(stdDev / DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE);
    }

public:
    void updateHistory(float temp, float humidity, float pressure) {
        tempHistory.push(temp);
        humidityHistory.push(humidity);
        pressureHistory.push(pressure);
    }

    void processSensorData(SensorData& data) {
        // Scale values
        data.scaled_temp = (data.temp - DuckConfig::MLConfig::MEAN_TEMP) / 
                           DuckConfig::MLConfig::STD_TEMP;
        data.scaled_humidity = (data.humidity - DuckConfig::MLConfig::MEAN_HUMIDITY) / 
                              DuckConfig::MLConfig::STD_HUMIDITY;
        data.scaled_pressure = (data.pressure - DuckConfig::MLConfig::MEAN_PRESS) / 
                              DuckConfig::MLConfig::STD_PRESS;

        // Update histories
        updateHistory(data.scaled_temp, data.scaled_humidity, data.scaled_pressure);

        // Compute volatilities
        data.temp_volatility = computeVolatility(tempHistory);
        data.humidity_volatility = computeVolatility(humidityHistory);
        data.pressure_volatility = computeVolatility(pressureHistory);

        // Compute velocities
        unsigned long currentTime = millis();
        if (lastSensorTime > 0) {
            float timeIntervalHours = (currentTime - lastSensorTime) / 3600000.0f;
            
            data.temp_velocity = (data.scaled_temp - prev_scaled_temp) / timeIntervalHours;
            data.humidity_velocity = (data.scaled_humidity - prev_scaled_humidity) / timeIntervalHours;
            data.pressure_velocity = (data.scaled_pressure - prev_scaled_pressure) / timeIntervalHours;
        }

        // Update tracking variables
        lastSensorTime = currentTime;
        prev_scaled_temp = data.scaled_temp;
        prev_scaled_humidity = data.scaled_humidity;
        prev_scaled_pressure = data.scaled_pressure;
    }
};

#endif // DUCK_SENSOR_H