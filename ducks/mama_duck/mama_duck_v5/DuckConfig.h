#ifndef DUCK_CONFIG_H
#define DUCK_CONFIG_H

#include <Arduino.h>

namespace DuckConfig {
    // System Configuration
    struct SystemConfig {
        // Time intervals
        //static const uint32_t SENSOR_READ_INTERVAL = 300000;  // 5 minutes -> not used
        static const uint32_t BME_READ_INTERVAL = 10000;      // 10 seconds
        static const uint32_t GPS_TIMEOUT = 5000;             // 5 seconds
        static const uint32_t TRANSMISSION_RETRY_DELAY = 1000; // 1 second
        
        // Buffer sizes
        static const size_t GPS_BUFFER_SIZE = 100;
        static const size_t MESSAGE_BUFFER_SIZE = 256;
        static const size_t HISTORY_WINDOW_SIZE = 5;
        
        // Task configuration
        static const uint32_t ML_STACK_SIZE = 32 * 1024;      // 32KB stack
        static const uint32_t TX_STACK_SIZE = 16 * 1024;      // 16KB stack
        static const uint32_t WDT_TIMEOUT = 3000;             // 3 seconds
        
        // Hardware configuration
        static const uint8_t LED_PIN = 4;
        static const uint8_t NUM_LEDS = 1;
        static const uint8_t LED_BRIGHTNESS = 128;
        
        // Retry configuration
        static const uint8_t MAX_RETRY_COUNT = 3;
    };

    // Sensor bounds for validation
    struct SensorBounds {
        static constexpr float MIN_TEMPERATURE = -40.0f;
        static constexpr float MAX_TEMPERATURE = 85.0f;
        static constexpr float MIN_HUMIDITY = 0.0f;
        static constexpr float MAX_HUMIDITY = 100.0f;
        static constexpr float MIN_PRESSURE = 300.0f;
        static constexpr float MAX_PRESSURE = 1100.0f;
        static constexpr float MIN_GAS = 0.0f;
        static constexpr float MAX_GAS = 200000.0f;
    };

    // ML Model configuration
    struct MLConfig {
        static constexpr float MEAN_TEMP = 29.43918594687232f;
        static constexpr float STD_TEMP = 2.474995698480596f;
        static constexpr float MEAN_HUMIDITY = 8.12150471293916f;
        static constexpr float STD_HUMIDITY = 0.7883001364721569f;
        static constexpr float MEAN_PRESS = 97206.94431876607f;
        static constexpr float STD_PRESS = 92.58890775783398f;
        static constexpr float MEAN_GAS = 68812.22262210795f;
        static constexpr float STD_GAS = 15230.956590523883f;
    };
};

#endif // DUCK_CONFIG_H