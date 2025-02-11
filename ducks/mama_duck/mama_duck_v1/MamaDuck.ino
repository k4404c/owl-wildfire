/**
 * @file MamaDuck.ino
 * @brief A MamaDuck that sends GPS coordinates every 60 seconds.
 */

#include <string>
#include <arduino-timer.h>
#include <CDP.h>
#include "FastLED.h"
#include "Zanshin_BME680.h"  // Include the BME680 Sensor library



// Setup for W2812 (LED)
#define LED_TYPE WS2812
#define DATA_PIN 4
#define NUM_LEDS 1
#define COLOR_ORDER GRB
#define BRIGHTNESS  128
#include <pixeltypes.h>
CRGB leds[NUM_LEDS];

#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

// GPS
#include <TinyGPS++.h>
TinyGPSPlus tgps;
HardwareSerial GPS(1);

// // Telemetry
// #include <axp20x.h>
// AXP20X_Class axp;

bool sendData(std::vector<byte> message, topics value);
bool runSensor(void *);
bool runBmeSensor(void *);
bool runGPS(void *);

// create a built-in mama duck
MamaDuck duck;

// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 300000;
int counter = 1;
bool setupOK = false;




/**************************************************************************************************
** Declare all program constants                                                                 **
**************************************************************************************************/
const uint32_t SERIAL_SPEED{115200};  ///< Set the baud rate for Serial I/O

/**************************************************************************************************
** Declare global variables and instantiate classes                                              **
**************************************************************************************************/
BME680_Class BME680;  ///< Create an instance of the BME680 class
///< Forward function declaration with default value for sea level
float altitude(const int32_t press, const float seaLevel = 1013.25);
float altitude(const int32_t press, const float seaLevel) {
  static float Altitude;
  Altitude =
      44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));  // Convert into meters
  return (Altitude);
}  // of method altitude()




void setup() {
  
  // LED Intial
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(  BRIGHTNESS );
  leds[0] = CRGB::Cyan;
  FastLED.show();

  //The Device ID must be exactly 8 bytes otherwise it will get rejected
  std::string deviceId("ASUMAMA1");
  std::vector<byte> devId;
  devId.insert(devId.end(), deviceId.begin(), deviceId.end());
  
  if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE) {
    Serial.println("[MAMA] Failed to setup MamaDuck");
    return;
  }

  // //Setup APX
  // Wire.begin(21, 22);
  // if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
  //   Serial.println("AXP192 Begin PASS");
  // } else {
  //   Serial.println("AXP192 Begin FAIL");
  // }
  // axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  // axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  // axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  // axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  // axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  
  //Setup GPS
  GPS.begin(9600, SERIAL_8N1, 34, 12);

  // Initialize the timer for telemetry
  timer.every(INTERVAL_MS, runSensor);
  timer.every(30000, runBmeSensor);


  #ifdef __AVR_ATmega32U4__      // If this is a 32U4 processor, then wait 3 seconds to init USB port
  delay(3000);
  #endif
  Serial.print(F("Starting I2CDemo example program for BME680\n"));
  Serial.print(F("- Initializing BME680 sensor\n"));
  while (!BME680.begin(I2C_STANDARD_MODE)) {  // Start BME680 using I2C, use first device found
    Serial.print(F("-  Unable to find BME680. Trying again in 5 seconds.\n"));
    delay(5000);
  }  // of loop until device is located
  Serial.print(F("- Setting 16x oversampling for all sensors\n"));
  BME680.setOversampling(TemperatureSensor, Oversample16);  // Use enumerated type values
  BME680.setOversampling(HumiditySensor, Oversample16);     // Use enumerated type values
  BME680.setOversampling(PressureSensor, Oversample16);     // Use enumerated type values
  Serial.print(F("- Setting IIR filter to a value of 4 samples\n"));
  BME680.setIIRFilter(IIR4);  // Use enumerated type values
  Serial.print(F("- Setting gas measurement to 320\xC2\xB0\x43 for 150ms\n"));  // "�C" symbols
  BME680.setGas(320, 150);  // 320�c for 150 milliseconds

  // LED Complete
  leds[0] = CRGB::Gold;
  FastLED.show();

  setupOK = true;

  Serial.println("[MAMA] Setup OK!");
}


std::vector<byte> stringToByteVector(const String& str) {
    std::vector<byte> byteVec;
    byteVec.reserve(str.length());

    for (unsigned int i = 0; i < str.length(); ++i) {
        byteVec.push_back(static_cast<byte>(str[i]));
    }

    return byteVec;
}

void loop() {
  if (!setupOK) {
    return; 
  }



  timer.tick();
  // Use the default run(). The Mama duck is designed to also forward data it receives
  // from other ducks, across the network. It has a basic routing mechanism built-in
  // to prevent messages from hoping endlessly.
  duck.run();
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available())
      tgps.encode(GPS.read());
  } while (millis() - start < ms);
}

// Getting GPS data
String getGPSData() {

  // Encoding the GPS
  smartDelay(5000);
  
  // Printing the GPS data
  Serial.println("[MAMA] --- GPS ---");
  Serial.print("[MAMA] Latitude  : ");
  Serial.println(tgps.location.lat(), 5);  
  Serial.print("[MAMA] Longitude : ");
  Serial.println(tgps.location.lng(), 4);
  Serial.print("[MAMA] Altitude  : ");
  Serial.print(tgps.altitude.feet() / 3.2808);
  Serial.println("M");
  Serial.print("[MAMA] Satellites: ");
  Serial.println(tgps.satellites.value());
  Serial.print("[MAMA] Time      : ");
  Serial.print(tgps.time.hour());
  Serial.print(":");
  Serial.print(tgps.time.minute());
  Serial.print(":");
  Serial.println(tgps.time.second());
  Serial.print("[MAMA] Speed     : ");
  Serial.println(tgps.speed.kmph());
  Serial.println("[MAMA] **********************");
  
  // Creating a message of the Latitude and Longitude
  String sensorVal = "Lat:" + String(tgps.location.lat(), 5) + " Lng:" + String(tgps.location.lng(), 4) + " Alt:" + String(tgps.altitude.feet() / 3.2808);

  // Check to see if GPS data is being received
  if (millis() > 5000 && tgps.charsProcessed() < 10)
  {
    Serial.println(F("[MAMA] No GPS data received: check wiring"));
  }

  return sensorVal;
}

bool runSensor(void *) {
  bool result;

  // String batteryData = getBatteryData();
  String gpsData = getGPSData();

  String message = String("Counter:") + String(counter)+ " " +String("FM:") + String(freeMemory())+ " " + gpsData; 
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message);

  result = sendData(stringToByteVector(message), location);
  if (result) {
     Serial.println("[MAMA] runSensor ok.");
  } else {
     Serial.println("[MAMA] runSensor failed.");
  }
  return result;
}

bool runBmeSensor(void *) {
  bool result;

  String message = "";
  String ftemp = "";
  String fhumidity = "";
  String fpressure = "";
  String faltitude = "";
  String fgas = "";


  static int32_t  temp, humidity, pressure, gas;  // BME readings
  static char     buf[16];                        // sprintf text buffer
  static float    alt;                            // Temporary variable
  static uint16_t loopCounter = 0;                // Display iterations
  if (loopCounter % 25 == 0) {                    // Show header @25 loops
    Serial.print(F("\nLoop Temp\xC2\xB0\x43 Humid% Press hPa   Alt m Air m"));
    Serial.print(F("\xE2\x84\xA6\n==== ====== ====== ========= ======= ======\n"));  // "�C" symbol
  }                                                     // if-then time to show headers
  BME680.getSensorData(temp, humidity, pressure, gas);  // Get readings

  sprintf(buf, "%4d %3d.%02d", (loopCounter - 1) % 9999,  // Clamp to 9999,
          (int8_t)(temp / 100), (uint8_t)(temp % 100));   // Temp in decidegrees
  Serial.print(buf);
  ftemp = buf;

  sprintf(buf, "%3d.%03d", (int8_t)(humidity / 1000),
          (uint16_t)(humidity % 1000));  // Humidity milli-pct
  Serial.print(buf);
  fhumidity = buf;

  sprintf(buf, "%7d.%02d", (int16_t)(pressure / 100),
          (uint8_t)(pressure % 100));  // Pressure Pascals
  Serial.print(buf);
  fpressure = buf;
  alt = altitude(pressure);                                                // temp altitude
  sprintf(buf, "%5d.%02d", (int16_t)(alt), ((uint8_t)(alt * 100) % 100));  // Altitude meters
  Serial.print(buf);
  faltitude = buf;

  sprintf(buf, "%4d.%02d\n", (int16_t)(gas / 100), (uint8_t)(gas % 100));  // Resistance milliohms
  Serial.print(buf);
  fgas = buf;

  message = String("Temp:") + " " + String(ftemp) + ", " + String("Humidity:") + " " + String(fhumidity) + ", " + String("Pressure:") + " " + String(fpressure) + ", " + String("Alt:") + " " + String(faltitude) + ", " + String("Gas:") + " " + String(fgas); 
  Serial.print("[MAMA] sensor data: ");
  Serial.println(message);
  delay(10000);  // Wait 10s


  result = sendData(stringToByteVector(message), location);
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

// // Getting the battery data
// String getBatteryData() {
  
//   int isCharging = axp.isChargeing();
//   boolean isFullyCharged = axp.isChargingDoneIRQ();
//   float batteryVoltage = axp.getBattVoltage();
//   float batteryDischarge = axp.getAcinCurrent();
//   float getTemp = axp.getTemp();  
//   float battPercentage = axp.getBattPercentage();
   
//   Serial.println("[MAMA] --- Power ---");
//   Serial.print("[MAMA] Duck charging (1 = Yes): ");
//   Serial.println(isCharging);
//   Serial.print("[MAMA] Fully Charged: ");
//   Serial.println(isFullyCharged);
//   Serial.print("[MAMA] Battery Voltage: ");
//   Serial.println(batteryVoltage);
//   Serial.print("[MAMA] Battery Discharge: ");
//   Serial.println(batteryDischarge);  
//   Serial.print("[MAMA] Battery Percentage: ");
//   Serial.println(battPercentage);
//   Serial.print("[MAMA] Board Temperature: ");
//   Serial.println(getTemp);
   
//   String sensorVal = 
//   "Charging:" + 
//   String(isCharging) +  
//   " Full:" +
//   String(isFullyCharged)+
//   " Volts:" +
//   String(batteryVoltage) + 
//   " Temp:" +
//   String(getTemp);

//   return sensorVal;
// }