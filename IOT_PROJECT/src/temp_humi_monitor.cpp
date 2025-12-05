#include "temp_humi_monitor.h"
#include "global.h"
#include <DHT20.h>

DHT20 dht20;

/**
 * @brief Task to read temperature & humidity from DHT20.
 *
 * Responsibilities:
 * - Read sensor values using I2C mutex protection.
 * - Update shared data (glob_temperature / glob_humidity).
 * - Update temperature state (Task 1).
 * - Update humidity state (Task 2).
 * - Update LCD state (Task 3).
 * - Print sensor data and CSV log.
 */
void temp_humi_monitor(void *pvParameters) {

    dht20.begin();
    Serial.println("Temp/Humi Task: Running...");

    while (true) {

        float temperature;
        float humidity;

        // -----------------------------
        // Read sensor via I2C (safe)
        // -----------------------------
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            dht20.read();
            temperature = dht20.getTemperature();
            humidity    = dht20.getHumidity();
            xSemaphoreGive(xI2CMutex);
        } else {
            Serial.println("[Sensor Task] ERROR: Failed to lock I2C mutex");
            temperature = humidity = -1.0f;
        }

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("[Sensor Task] ERROR: Invalid DHT20 reading");
            temperature = humidity = -1.0f;
        }

        // ---------------------------------------
        // Store sensor values (shared variables)
        // ---------------------------------------
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            glob_temperature = temperature;
            glob_humidity    = humidity;
            xSemaphoreGive(xDataMutex);
        }

        // -----------------------------
        // Update temperature state
        // -----------------------------
        TemperatureState_t newTempState =
            (temperature < 25.0f) ? STATE_COLD :
            (temperature < 32.0f) ? STATE_NORMAL :
                                    STATE_HOT;

        if (xSemaphoreTake(xTempStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (g_currentTempState != newTempState) {
                g_currentTempState = newTempState;
                Serial.printf("[Sensor Task] Temp State: %s (%.1f°C)\n",
                        (newTempState == STATE_COLD   ? "COLD" :
                         newTempState == STATE_NORMAL ? "NORMAL" : "HOT"),
                        temperature);
            }
            xSemaphoreGive(xTempStateMutex);
        }

        // -----------------------------
        // Update humidity state
        // -----------------------------
        HumidityState_t newHumiState =
            (humidity < 55.0f) ? STATE_DRY :
            (humidity < 80.0f) ? STATE_COMFORT :
                                 STATE_WET;

        if (xSemaphoreTake(xHumiStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (g_currentHumiState != newHumiState) {
                g_currentHumiState = newHumiState;
                Serial.printf("[Sensor Task] Humi State: %s (%.1f%%)\n",
                        (newHumiState == STATE_DRY     ? "DRY" :
                         newHumiState == STATE_COMFORT ? "COMFORT" : "WET"),
                        humidity);
            }
            xSemaphoreGive(xHumiStateMutex);
        }

        // -----------------------------
        // Update LCD state (Task 3)
        // -----------------------------
        LcdState_t newLcdState;

        if (temperature > 32.0f || humidity > 80.0f) {
            newLcdState = LCD_CRITICAL;
        }
        else if (temperature > 30.0f || humidity > 70.0f) {
            newLcdState = LCD_WARNING;
        }
        else {
            newLcdState = LCD_NORMAL;
        }

        if (xSemaphoreTake(xLcdStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (g_currentLcdState != newLcdState) {
                g_currentLcdState = newLcdState;
                Serial.printf("[Sensor Task] LCD State: %s\n",
                              (newLcdState == LCD_CRITICAL ? "CRITICAL" :
                               newLcdState == LCD_WARNING  ? "WARNING" :
                                                              "NORMAL"));
            }
            xSemaphoreGive(xLcdStateMutex);
        }

        // -----------------------------
        // Print sensor values & CSV log
        // -----------------------------
        Serial.printf("Humidity: %.2f%%  Temperature: %.2f°C\n",
                      humidity, temperature);

        bool isNormal =
            (temperature >= 25.0f && temperature < 32.0f) &&
            (humidity    >= 40.0f && humidity    < 80.0f);

        Serial.printf("%.1f,%.1f,%s\n",
                      temperature,
                      humidity,
                      isNormal ? "normal" : "anomaly");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
