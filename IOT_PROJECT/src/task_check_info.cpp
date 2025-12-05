#include "task_check_info.h"
#include "task_wifi.h"

// Load configuration from /info.dat into global variables
void Load_info_File() {
    File file = LittleFS.open("/info.dat", "r");
    if (!file) {
        Serial.println("[Load] /info.dat not found.");
        return;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print(F("[Load] deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    WIFI_SSID       = String((const char *)doc["WIFI_SSID"]);
    WIFI_PASS       = String((const char *)doc["WIFI_PASS"]);
    CORE_IOT_TOKEN  = String((const char *)doc["CORE_IOT_TOKEN"]);
    CORE_IOT_SERVER = String((const char *)doc["CORE_IOT_SERVER"]);
    CORE_IOT_PORT   = String((const char *)doc["CORE_IOT_PORT"]);

    Serial.println("[Load] Loaded config from /info.dat:");
    Serial.println("  WIFI_SSID       = " + WIFI_SSID);
    Serial.println("  WIFI_PASS       = " + WIFI_PASS);
    Serial.println("  CORE_IOT_TOKEN  = " + CORE_IOT_TOKEN);
    Serial.println("  CORE_IOT_SERVER = " + CORE_IOT_SERVER);
    Serial.println("  CORE_IOT_PORT   = " + CORE_IOT_PORT);
}

// Delete config file and restart (used by BOOT button task)
void Delete_info_File() {
    if (LittleFS.exists("/info.dat")) {
        LittleFS.remove("/info.dat");
    }
    ESP.restart();
}

// Save Wi-Fi/CoreIOT config to /info.dat then restart
void Save_info_File(String wifi_ssid,
                    String wifi_pass,
                    String core_token,
                    String core_server,
                    String core_port) {
    Serial.println("[Save] Save_info_File() called");
    Serial.println("  SSID   = " + wifi_ssid);
    Serial.println("  PASS   = " + wifi_pass);

    if (!LittleFS.begin(true)) {
        Serial.println("[Save] LittleFS.begin FAILED");
        return;
    }

    DynamicJsonDocument doc(4096);
    doc["WIFI_SSID"]       = wifi_ssid;
    doc["WIFI_PASS"]       = wifi_pass;
    doc["CORE_IOT_TOKEN"]  = core_token;
    doc["CORE_IOT_SERVER"] = core_server;
    doc["CORE_IOT_PORT"]   = core_port;

    File configFile = LittleFS.open("/info.dat", "w");
    if (!configFile) {
        Serial.println("[Save] open('/info.dat','w') FAILED");
        return;
    }

    size_t n = serializeJson(doc, configFile);
    configFile.close();
    Serial.printf("[Save] Wrote %u bytes to /info.dat\n", (unsigned)n);

    Serial.println("[Save] Restarting ESP to apply new configuration...");
    delay(500);
    ESP.restart();
}

// Check whether config exists; if not, start AP mode
bool check_info_File(bool check) {
    if (!check) {
        if (!LittleFS.begin(true)) {
            Serial.println("❌ LittleFS init failed!");
            return false;
        }
        Load_info_File();
    }

    if (WIFI_SSID.isEmpty() && WIFI_PASS.isEmpty()) {
        if (!check) {
            startAP();
        }
        return false;
    }

    return true;
}
