#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// -----------------------------------------------------------------------------
// Access Point configuration (used when device has no saved WiFi credentials)
// -----------------------------------------------------------------------------
#ifndef SSID_AP
  #define SSID_AP  "ESP32-AP"
#endif

#ifndef PASS_AP
  #define PASS_AP  "12345678"     // Minimum 8 characters
#endif

// -----------------------------------------------------------------------------
// GPIO used for manual device control (Task 1 / Task 2)
// -----------------------------------------------------------------------------
#ifndef LED1_PIN
  #define LED1_PIN 48             // Example LED or relay
#endif

#ifndef LED2_PIN
  #define LED2_PIN 41             // Example LED or relay
#endif

extern bool blinkEnabled;
extern bool neoEnabled;

// -----------------------------------------------------------------------------
// Shared sensor data
// -----------------------------------------------------------------------------
extern float glob_temperature;
extern float glob_humidity;

// -----------------------------------------------------------------------------
// WiFi + CoreIOT credentials (loaded from /info.dat)
// -----------------------------------------------------------------------------
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

// -----------------------------------------------------------------------------
// Temperature state (Task 1)
// -----------------------------------------------------------------------------
typedef enum {
    STATE_COLD,
    STATE_NORMAL,
    STATE_HOT
} TemperatureState_t;

extern SemaphoreHandle_t    xTempStateMutex;
extern TemperatureState_t   g_currentTempState;

// -----------------------------------------------------------------------------
// Humidity state (Task 2)
// -----------------------------------------------------------------------------
typedef enum {
    STATE_DRY,
    STATE_COMFORT,
    STATE_WET
} HumidityState_t;

extern SemaphoreHandle_t   xHumiStateMutex;
extern HumidityState_t     g_currentHumiState;

// -----------------------------------------------------------------------------
// LCD state (Task 3)
// -----------------------------------------------------------------------------
typedef enum {
    LCD_NORMAL,
    LCD_WARNING,
    LCD_CRITICAL
} LcdState_t;

extern SemaphoreHandle_t xLcdStateMutex;
extern LcdState_t        g_currentLcdState;

// -----------------------------------------------------------------------------
// Mutexes for sensor data and I2C access
// -----------------------------------------------------------------------------
extern SemaphoreHandle_t xDataMutex;
extern SemaphoreHandle_t xI2CMutex;

#endif // __GLOBAL_H__
