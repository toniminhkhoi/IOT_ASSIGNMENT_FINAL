#include "neo_blinky.h"
#include "global.h"

// Global NeoPixel strip instance
Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

/**
 * @brief NeoPixel task.
 *
 * Uses the shared humidity state (Task 2) to set LED color:
 * - STATE_DRY     → Yellow
 * - STATE_COMFORT → Green
 * - STATE_WET     → Blue
 *
 * If neoEnabled == false, the LED is turned off.
 */
void neo_blinky(void *pvParameters) {
    strip.begin();
    strip.clear();
    strip.show();

    Serial.println("NeoPixel Task: Running...");

    HumidityState_t localState;

    while (true) {

        // Safely read humidity state from shared variable
        if (xSemaphoreTake(xHumiStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            localState = g_currentHumiState;
            xSemaphoreGive(xHumiStateMutex);
        }

        if (neoEnabled) {
            switch (localState) {
                case STATE_DRY:
                    // Dry: yellow
                    strip.setPixelColor(0, strip.Color(255, 255, 0));
                    break;

                case STATE_COMFORT:
                    // Comfortable: green
                    strip.setPixelColor(0, strip.Color(0, 255, 0));
                    break;

                case STATE_WET:
                    // Wet: blue
                    strip.setPixelColor(0, strip.Color(0, 0, 255));
                    break;
            }
        } else {
            // Feature disabled → turn NeoPixel off
            strip.clear();
        }

        strip.show();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
