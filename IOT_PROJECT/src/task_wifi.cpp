#include "task_wifi.h"
#include "global.h"
#include <WiFi.h>

// Start ESP32 in soft-AP mode for configuration portal
void startAP() {
    Serial.println(">>> SSID_AP   = " + String(SSID_AP));
    Serial.println(">>> WIFI_SSID = " + String(WIFI_SSID));

    WiFi.mode(WIFI_AP);
    WiFi.softAP(String(SSID_AP), String(PASS_AP));

    Serial.print("AP started with SSID: ");
    Serial.println(SSID_AP);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

// RTOS task: connect ESP32 in STA mode to Wi-Fi
void Task_Wifi(void *pvParameters) {
    Serial.println("Task Wifi: running...");

    // Wait until WiFi credentials are loaded from /info.dat
    while (WIFI_SSID.isEmpty()) {
        Serial.println("Task Wifi: waiting for WIFI_SSID...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    Serial.printf("Task Wifi: connecting to %s...\n", WIFI_SSID.c_str());
    WiFi.mode(WIFI_STA);

    if (WIFI_PASS.isEmpty()) {
        WiFi.begin(WIFI_SSID.c_str());
    } else {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        Serial.print(".");
    }

    Serial.println("\nTask Wifi: WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    isWifiConnected = true;

    // Notify other tasks (CoreIOT) that internet is ready
    xSemaphoreGive(xBinarySemaphoreInternet);

    // Done, delete this task
    vTaskDelete(nullptr);
}
