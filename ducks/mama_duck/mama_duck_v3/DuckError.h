#ifndef DUCK_ERROR_H
#define DUCK_ERROR_H

#include <Arduino.h>
#include <functional>
#include "DuckConfig.h"

enum class DuckStatus {
    OK,
    ERROR_BME680_INIT,
    ERROR_GPS_INIT,
    ERROR_RADIO_INIT,
    ERROR_QUEUE_FULL,
    ERROR_SENSOR_READ,
    ERROR_GPS_TIMEOUT,
    ERROR_TRANSMISSION,
    WARNING_LOW_MEMORY,
    WARNING_GPS_NO_FIX
};

class DuckErrorHandler {
private:
    static DuckStatus currentStatus;
    static char lastErrorMessage[50];

public:
    static void setError(DuckStatus status, const char* message = nullptr) {
        currentStatus = status;
        if (message) {
            strncpy(lastErrorMessage, message, sizeof(lastErrorMessage) - 1);
            lastErrorMessage[sizeof(lastErrorMessage) - 1] = '\0';
        }
        // Log error
        Serial.printf("[MAMA] Error: %s\n", message ? message : "Unknown error");
    }

    static bool isError() {
        return currentStatus != DuckStatus::OK;
    }

    static DuckStatus getStatus() {
        return currentStatus;
    }

    // Modified retry function to accept std::function
    static bool retry(const char* operation, std::function<bool()> func, uint8_t maxRetries = DuckConfig::SystemConfig::MAX_RETRY_COUNT) {
        uint8_t attempts = 0;
        while (attempts < maxRetries) {
            if (func()) {
                if (attempts > 0) {
                    Serial.printf("[MAMA] %s succeeded after %d retries\n", operation, attempts);
                }
                return true;
            }
            attempts++;
            if (attempts < maxRetries) {
                Serial.printf("[MAMA] %s failed, attempt %d/%d\n", operation, attempts, maxRetries);
                delay(DuckConfig::SystemConfig::TRANSMISSION_RETRY_DELAY);
            }
        }
        setError(DuckStatus::ERROR_SENSOR_READ, operation);
        return false;
    }

    static void checkStackHealth(TaskHandle_t task, const char* taskName, size_t minStackWatermark) {
        UBaseType_t watermark = uxTaskGetStackHighWaterMark(task);
        if (watermark < minStackWatermark) {
            Serial.printf("[MAMA] WARNING: %s stack watermark low: %d bytes\n", taskName, watermark);
            setError(DuckStatus::WARNING_LOW_MEMORY);
        }
    }
};

// Initialize static members
DuckStatus DuckErrorHandler::currentStatus = DuckStatus::OK;
char DuckErrorHandler::lastErrorMessage[50] = {0};

#endif // DUCK_ERROR_H