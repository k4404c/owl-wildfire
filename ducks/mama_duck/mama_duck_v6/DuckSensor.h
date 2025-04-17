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
    float scaled_gas;
    float temp_volatility;
    float humidity_volatility;
    float pressure_volatility;
    float gas_volatility;
    float temp_velocity;
    float humidity_velocity;
    float pressure_velocity;
    float gas_velocity;
    
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
    CircularBuffer<float, DuckConfig::SystemConfig::HISTORY_WINDOW_SIZE> gasHistory;
    
    float prev_temp = 0.0f;
    float prev_humidity = 0.0f;
    float prev_pressure = 0.0f;
    float prev_gas = 0.0f;
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
    void updateHistory(float temp, float humidity, float pressure, float gas) {
        tempHistory.push(temp);
        humidityHistory.push(humidity);
        pressureHistory.push(pressure);
        gasHistory.push(gas);
    }


    void processSensorData(SensorData& data) {
        // Scale values
        data.scaled_temp = (data.temp - DuckConfig::MLConfig::MEAN_TEMP) / 
                           DuckConfig::MLConfig::STD_TEMP;
        data.scaled_humidity = (data.humidity - DuckConfig::MLConfig::MEAN_HUMIDITY) / 
                              DuckConfig::MLConfig::STD_HUMIDITY;
        data.scaled_pressure = (data.pressure - DuckConfig::MLConfig::MEAN_PRESS) / 
                              DuckConfig::MLConfig::STD_PRESS;
        data.scaled_gas = (data.gas - DuckConfig::MLConfig::MEAN_GAS) / 
                         DuckConfig::MLConfig::STD_GAS;

        // Update histories
        updateHistory(data.temp, data.humidity, data.pressure, data.gas);

        // Compute volatilities
        data.temp_volatility = computeVolatility(tempHistory);
        data.humidity_volatility = computeVolatility(humidityHistory);
        data.pressure_volatility = computeVolatility(pressureHistory);
        data.gas_volatility = computeVolatility(gasHistory);


        // Compute velocities
        unsigned long currentTime = millis();
        if (lastSensorTime > 0) {
            float timeIntervalHours = (currentTime - lastSensorTime) / 3600000.0f;
            
            data.temp_velocity = (data.temp - prev_temp) / timeIntervalHours;
            data.humidity_velocity = (data.humidity - prev_humidity) / timeIntervalHours;
            data.pressure_velocity = (data.pressure - prev_pressure) / timeIntervalHours;
            data.gas_velocity = (data.gas - prev_gas) / timeIntervalHours;
        }

        // Update tracking variables
        lastSensorTime = currentTime;
        prev_temp = data.temp;
        prev_humidity = data.humidity;
        prev_pressure = data.pressure;
        prev_gas = data.gas;
    }
};

#endif // DUCK_SENSOR_H