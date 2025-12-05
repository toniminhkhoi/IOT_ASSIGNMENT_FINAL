#include "task_handler.h"
#include "global.h"
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>

void Webserver_sendata(String data);  // WebSocket broadcast helper

// GPIO mapping for web UI
static const int BLINK_GPIO = 41;  // External Blink LED (device 1)
static const int NEO_GPIO   = 48;  // External NeoPixel (device 2)

// NeoPixel on GPIO48
Adafruit_NeoPixel pixels(1, NEO_GPIO, NEO_GRB + NEO_KHZ800);
static bool neoInited = false;

static void ensureNeoInit() {
    if (!neoInited) {
        pixels.begin();
        pixels.setBrightness(50);
        pixels.clear();
        pixels.show();
        neoInited = true;
    }
}

void turnNeoPixel(bool on) {
    ensureNeoInit();
    if (on) {
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));   // red
    } else {
        pixels.setPixelColor(0, 0);                         // off
    }
    pixels.show();

    Serial.printf("NeoPixel (%d) => %s\n", NEO_GPIO, on ? "ON" : "OFF");
}

void turnBlinkLed(bool on) {
    pinMode(BLINK_GPIO, OUTPUT);
    digitalWrite(BLINK_GPIO, on ? HIGH : LOW);
    Serial.printf("Blink LED (%d) => %s\n", BLINK_GPIO, on ? "ON" : "OFF");
}

// Handle incoming WebSocket JSON messages
void handleWebSocketMessage(String message) {
    Serial.println("📩 WS Message: " + message);

    JSONVar json = JSON.parse(message);
    if (JSON.typeof(json) == "undefined") {
        Serial.println("❌ JSON parse failed");
        return;
    }

    String page = (const char *)json["page"];

    // 1. Device control page (turn LED / NeoPixel on/off)
    if (page == "device") {
        JSONVar v     = json["value"];
        int gpio      = (int)v["gpio"];
        String state  = (const char *)v["state"];
        bool turnOn   = (state == "ON");

        if (gpio == BLINK_GPIO) {
            blinkEnabled = turnOn;
            Serial.printf("🌕 Blink LED %s\n", turnOn ? "ON" : "OFF");
        } else if (gpio == NEO_GPIO) {
            neoEnabled = turnOn;
            Serial.printf("🌈 NeoPixel %s\n", turnOn ? "ON" : "OFF");
        }

        // Send acknowledgement back to web UI
        String ack = "{\"page\":\"device_ack\",\"gpio\":" + String(gpio) +
                     ",\"state\":\"" + state + "\"}";
        Webserver_sendata(ack);
    }

    // 2. Settings page: Wi-Fi / CoreIOT configuration
    if (page == "setting") {
        JSONVar v = json["value"];

        // Read from JSON
        String newSsid   = (const char *)v["ssid"];
        String newPass   = (const char *)v["password"];
        String newToken  = (const char *)v["token"];
        String newServer = (const char *)v["server"];
        String newPort   = (const char *)v["port"];

        // Store into globals
        WIFI_SSID       = newSsid;
        WIFI_PASS       = newPass;
        CORE_IOT_TOKEN  = newToken;
        CORE_IOT_SERVER = newServer;
        CORE_IOT_PORT   = newPort;

        // Log for debugging
        Serial.println("📌 Update Wi-Fi/CoreIOT config:");
        Serial.println("  SSID   = " + WIFI_SSID);
        Serial.println("  PASS   = " + WIFI_PASS);
        Serial.println("  TOKEN  = " + CORE_IOT_TOKEN);
        Serial.println("  SERVER = " + CORE_IOT_SERVER);
        Serial.println("  PORT   = " + CORE_IOT_PORT);

        // Save to /info.dat and restart
        Save_info_File(newSsid, newPass, newToken, newServer, newPort);
    }
}
