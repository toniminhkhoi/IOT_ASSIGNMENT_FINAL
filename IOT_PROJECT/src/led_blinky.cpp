#include "led_blinky.h"
#include "global.h"

/**
 * @brief LED blinking task.
 * Blinks LED according to the current temperature state.
 * - STATE_COLD   → slow blink
 * - STATE_NORMAL → heartbeat blink
 * - STATE_HOT    → fast blink
 * If blinkEnabled == false → LED stays OFF.
 */
void led_blinky(void *pvParameters) {
    pinMode(LED_GPIO, OUTPUT);
    Serial.println("LED Blink Task: Running...");

    TemperatureState_t localState;

    while (true) {

        // Safely read shared temperature state
        if (xSemaphoreTake(xTempStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            localState = g_currentTempState;
            xSemaphoreGive(xTempStateMutex);
        }

        if (blinkEnabled) {
            switch (localState) {

                case STATE_COLD:
                    // Slow blink
                    digitalWrite(LED_GPIO, HIGH);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    digitalWrite(LED_GPIO, LOW);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    break;

                case STATE_NORMAL:
                    // Heartbeat pattern
                    digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(100));
                    digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(100));
                    digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(100));
                    digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(700));
                    break;

                case STATE_HOT:
                    // Fast blink
                    digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(100));
                    digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(100));
                    break;
            }
        }
        else {
            // LED disabled
            digitalWrite(LED_GPIO, LOW);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}
