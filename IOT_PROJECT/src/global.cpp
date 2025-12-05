#include "global.h"

// Shared sensor values
float glob_temperature = 0.0f;
float glob_humidity    = 0.0f;

// WiFi + CoreIOT credentials (loaded from /info.dat)
String WIFI_SSID;
String WIFI_PASS;
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

// Device control flags
bool blinkEnabled = true;
bool neoEnabled   = true;

// WiFi connection state
boolean isWifiConnected = false;

// Semaphores (created in main.cpp)
SemaphoreHandle_t xBinarySemaphoreInternet;
SemaphoreHandle_t xI2CMutex;

// Extra (unused) variables left intact intentionally
String wifi_ssid     = "YOLOuno";
String wifi_password = "12345678";
