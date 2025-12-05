#ifndef __LED_BLINKY__
#define __LED_BLINKY__

#include <Arduino.h>
#include "global.h"

// LED GPIO for Task 1 (temperature-based blinking)
#define LED_GPIO 48

void led_blinky(void *pvParameters);

#endif
