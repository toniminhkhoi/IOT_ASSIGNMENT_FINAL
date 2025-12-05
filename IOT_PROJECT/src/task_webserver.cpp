#include "task_webserver.h"
#include "global.h"
#include "task_wifi.h"        // startAP()
#include "task_handler.h"     // handleWebSocketMessage(...)
#include <WiFi.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

static bool webserverIsRunning = false;

// Broadcast data to all connected WebSocket clients
void Webserver_sendata(String data) {
    if (ws.count() > 0) {
        ws.textAll(data);
        Serial.println("📤 WS broadcast: " + data);
    } else {
        Serial.println("⚠️ No WebSocket clients connected");
    }
}

// WebSocket event handler
static void onEvent(AsyncWebSocket *serverPtr,
                    AsyncWebSocketClient *client,
                    AwsEventType type,
                    void *arg,
                    uint8_t *data,
                    size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WS client #%u connected from %s\n",
                      client->id(),
                      client->remoteIP().toString().c_str());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WS client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT) {
            String message;
            message += String((char *)data).substring(0, len);
            // Delegate to project handler (GPIO control / config saving)
            handleWebSocketMessage(message);
        }
    }
}

// Register routes and start web server
void connnectWSV() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Static files from LittleFS (/data)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/script.js", "application/javascript");
    });
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/styles.css", "text/css");
    });

    // Sensor API: read with mutex for RTOS safety
    server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
        float t, h;

        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            t = glob_temperature;
            h = glob_humidity;
            xSemaphoreGive(xDataMutex);
        } else {
            // Fallback if we cannot acquire mutex in time
            t = glob_temperature;
            h = glob_humidity;
        }

        AsyncResponseStream *res = request->beginResponseStream("application/json");
        res->printf("{\"temp\":%.2f,\"hum\":%.2f}", t, h);
        request->send(res);
    });

    server.begin();
    ElegantOTA.begin(&server);
    webserverIsRunning = true;
}

// Stop web server
void Webserver_stop() {
    ws.closeAll();
    server.end();
    webserverIsRunning = false;
}

// Ensure server is running and service ElegantOTA loop
void Webserver_reconnect() {
    if (!webserverIsRunning) {
        connnectWSV();
    }
    ElegantOTA.loop();
}

// RTOS task: run WebServer in AP mode
void Task_Webserver(void *pvParameters) {
    // Initialize IO for 2 external devices (LED/relay)
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);

    // Safe to call multiple times
    LittleFS.begin(true);

    // Start AP and web server (SSID/PASS from global.h → task_wifi.cpp)
    startAP();
    connnectWSV();

    for (;;) {
        Webserver_reconnect();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
