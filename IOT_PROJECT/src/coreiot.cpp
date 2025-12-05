#include "coreiot.h"
#include "global.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 

// WiFi + MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

// ==========================================================
//  MQTT Callback - handle RPC from CoreIOT
// ==========================================================
void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("[CoreIOT] Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    // Convert payload to String
    String msg;
    msg.reserve(length + 1);
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.println(msg);

    // Parse JSON
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, msg);
    if (err) {
        Serial.print("[CoreIOT] JSON parse error: ");
        Serial.println(err.c_str());
        return;
    }

    String topicStr(topic);

    // ------------------------------------------------------
    // Handle RPC calls: v1/devices/me/rpc/request/<requestId>
    // ------------------------------------------------------
    if (topicStr.startsWith("v1/devices/me/rpc/request/")) {
        // Extract requestId from topic
        int lastSlash = topicStr.lastIndexOf('/');
        String requestId = topicStr.substring(lastSlash + 1);

        const char *method = doc["method"];
        JsonVariant params = doc["params"];

        if (!method) {
            Serial.println("[CoreIOT] RPC has no method field");
            return;
        }

        Serial.print("[CoreIOT] RPC method: ");
        Serial.println(method);

        // Toggle blinkEnabled
        if (strcmp(method, "setBlinkEnabled") == 0 && !params.isNull()) {
            bool value = params.as<bool>();
            blinkEnabled = value;
            Serial.printf("[CoreIOT] blinkEnabled set to %s\n",
                          blinkEnabled ? "true" : "false");
        }

        // Toggle neoEnabled
        else if (strcmp(method, "setNeoEnabled") == 0 && !params.isNull()) {
            bool value = params.as<bool>();
            neoEnabled = value;
            Serial.printf("[CoreIOT] neoEnabled set to %s\n",
                          neoEnabled ? "true" : "false");
        }

        // Send simple RPC response (ack)
        StaticJsonDocument<64> resp;
        resp["result"] = true;
        char respBuf[64];
        size_t n = serializeJson(resp, respBuf);

        String respTopic = "v1/devices/me/rpc/response/" + requestId;
        client.publish(respTopic.c_str(), respBuf, n);
        Serial.print("[CoreIOT] RPC response published to ");
        Serial.println(respTopic);
    }

}

// ==========================================================
//  MQTT Reconnect Loop
// ==========================================================
void reconnect() {
    while (!client.connected()) {
        Serial.println("[CoreIOT] Attempting MQTT connection...");

        // Connect using device TOKEN as username
        bool ok = client.connect("ESP32Client",
                                 CORE_IOT_TOKEN.c_str(),
                                 "");

        Serial.print("[CoreIOT] connect() returned = ");
        Serial.println(ok ? "true" : "false");

        Serial.print("[CoreIOT] client.state() = ");
        Serial.println(client.state());

        if (ok) {
            Serial.println("[CoreIOT] MQTT connected!");

            // Subscribe to RPC requests
            client.subscribe("v1/devices/me/rpc/request/+");
            Serial.println("[CoreIOT] Subscribed to RPC topic");

            // (Optional) publish current LED/NeoPixel states as attributes
            StaticJsonDocument<128> attrDoc;
            attrDoc["blinkEnabled"] = blinkEnabled;
            attrDoc["neoEnabled"]   = neoEnabled;

            char attrBuf[128];
            size_t len = serializeJson(attrDoc, attrBuf);
            bool attrOk = client.publish("v1/devices/me/attributes", attrBuf, len);
            Serial.print("[CoreIOT] Initial attributes sent: ");
            Serial.println(attrOk ? "OK" : "FAILED");
        } else {
            Serial.println("[CoreIOT] MQTT connect failed, retry in 5 seconds...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

// ==========================================================
//  Setup CoreIOT (wait for WiFi STA connection)
// ==========================================================
static void setup_coreiot() {
    Serial.println("[CoreIOT] Waiting for WiFi (STA mode)...");

    // Wait for Task_Wifi to signal that WiFi is connected
    if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY) == pdTRUE) {
        Serial.println("[CoreIOT] WiFi is ready, configuring MQTT...");
    } else {
        Serial.println("[CoreIOT] ERROR: Semaphore wait failed");
        return;
    }

    // Print loaded configuration
    Serial.println("[CoreIOT] Using configuration:");
    Serial.println("  SERVER : " + CORE_IOT_SERVER);
    Serial.println("  PORT   : " + CORE_IOT_PORT);
    Serial.println("  TOKEN  : " + CORE_IOT_TOKEN);

    // Configure MQTT client
    client.setServer(CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.toInt());
    client.setCallback(callback);
}

// ==========================================================
//  CoreIOT RTOS Task — publish sensor telemetry every 10s
// ==========================================================
void coreiot_task(void *pvParameters) {
    Serial.println("[CoreIOT] Task started");
    setup_coreiot();

    // Safety check — missing configuration
    if (CORE_IOT_SERVER.length() == 0 ||
        CORE_IOT_PORT.length()   == 0)
    {
        Serial.println("[CoreIOT] ERROR: Server/Port missing. Task stopped.");
        vTaskDelete(nullptr);
        return;
    }

    for (;;) {

        // Ensure MQTT is connected
        if (!client.connected()) {
            reconnect();
        }
        client.loop();

        // Acquire sensor data safely
        float temp, humi;
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            temp = glob_temperature;
            humi = glob_humidity;
            xSemaphoreGive(xDataMutex);
        } else {
            temp = humi = -1000.0f;
        }

        // Validity check
        bool valid =
            !isnan(temp) && !isnan(humi) &&
            temp > -40.0f && temp < 125.0f &&
            humi >= 0.0f && humi <= 100.0f;

        if (valid) {
            // Build telemetry JSON
            String payload = "{\"temperature\":";
            payload += String(temp, 2);
            payload += ",\"humidity\":";
            payload += String(humi, 2);
            payload += "}";

            bool ok = client.publish("v1/devices/me/telemetry", payload.c_str());

            Serial.print("[CoreIOT] Telemetry sent: ");
            Serial.println(payload);

            Serial.println(ok ?
                "[CoreIOT] Publish OK" :
                "[CoreIOT] Publish FAILED");
        } else {
            Serial.println("[CoreIOT] Invalid sensor data, skipped");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // every 10 seconds
    }
}
