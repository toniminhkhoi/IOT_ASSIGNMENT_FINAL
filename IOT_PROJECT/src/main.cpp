#include "global.h"

#include <Wire.h>

// Tasks
#include "lcd_display.h"
#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "tinyml.h"
#include "coreiot.h"
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"

// -----------------------------------------------------------------------------
// Global semaphores and shared state
// -----------------------------------------------------------------------------
SemaphoreHandle_t xTempStateMutex;
SemaphoreHandle_t xHumiStateMutex;
SemaphoreHandle_t xLcdStateMutex;
SemaphoreHandle_t xDataMutex;

TemperatureState_t g_currentTempState = STATE_NORMAL;
HumidityState_t    g_currentHumiState = STATE_COMFORT;
LcdState_t         g_currentLcdState  = LCD_NORMAL;

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Wire.begin(11, 12);

    // ---------------------------
    // Create all required Mutexes
    // ---------------------------
    xTempStateMutex          = xSemaphoreCreateMutex();
    xBinarySemaphoreInternet = xSemaphoreCreateBinary();
    xHumiStateMutex          = xSemaphoreCreateMutex();
    xLcdStateMutex           = xSemaphoreCreateMutex();
    xDataMutex               = xSemaphoreCreateMutex();
    xI2CMutex                = xSemaphoreCreateMutex();

    // Verify successful creation
    if (!xTempStateMutex ||
        !xHumiStateMutex ||
        !xBinarySemaphoreInternet ||
        !xLcdStateMutex ||
        !xDataMutex ||
        !xI2CMutex)
    {
        Serial.println("ERROR: Failed to create one or more mutexes!");
        while (1) vTaskDelay(1000);
    }

    Serial.println("All Mutexes and Semaphores created successfully.");

    // Task to handle BOOT button reset
    xTaskCreate(Task_Toogle_BOOT, "Task Boot Button", 4096, NULL, 5, NULL);

    // Load credentials from LittleFS
    bool credentials_exist = check_info_File(false);

    // -------------------------------------------------------------------------
    // CLIENT MODE: WiFi credentials found → connect to WiFi + CoreIOT
    // -------------------------------------------------------------------------
    if (credentials_exist) {
        Serial.println("WiFi credentials found. Starting in CLIENT mode.");

        xTaskCreate(Task_Wifi, "Task Wifi", 4096, NULL, 3, NULL); // WiFi + Internet ready signal

        xTaskCreate(led_blinky,        "Task LED Blink", 2048, NULL, 2, NULL);
        xTaskCreate(neo_blinky,        "Task NEO Blink", 2048, NULL, 2, NULL);
          xTaskCreate(temp_humi_monitor, "Task TEMP HUMI", 4096, NULL, 2, NULL);
        xTaskCreate(tiny_ml_task,      "TinyML",         4096, NULL, 2, NULL);
        xTaskCreate(coreiot_task,      "CoreIOT Task",   4096, NULL, 2, NULL);
        xTaskCreate(lcd_display_task,  "Task LCD",       4096, NULL, 2, NULL);
    }
    // -------------------------------------------------------------------------
    // AP MODE: No WiFi credentials → start Webserver AP
    // -------------------------------------------------------------------------
    else {
        Serial.println("No WiFi credentials found. Starting in ACCESS POINT mode.");

        xTaskCreate(Task_Webserver,    "Task Webserver", 4096, NULL, 3, NULL);

        xTaskCreate(led_blinky,        "Task LED Blink", 2048, NULL, 2, NULL);
        xTaskCreate(neo_blinky,        "Task NEO Blink", 2048, NULL, 2, NULL);
        xTaskCreate(temp_humi_monitor, "Task TEMP HUMI", 4096, NULL, 2, NULL);
        xTaskCreate(tiny_ml_task,      "TinyML",         4096, NULL, 2, NULL);
        xTaskCreate(lcd_display_task,  "Task LCD",       4096, NULL, 2, NULL);
    }
}

// -----------------------------------------------------------------------------
// Loop (unused because FreeRTOS is running)
// -----------------------------------------------------------------------------
void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
