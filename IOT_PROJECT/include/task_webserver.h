#ifndef __TASK_WEBSERVER_H__
#define __TASK_WEBSERVER_H__

#include <Arduino.h>
#include <ESPAsyncWebServer.h> // Thêm các thư viện phụ thuộc
#include <ElegantOTA.h>
#include <AsyncWebSocket.h>
// Đối tượng server/websocket được định nghĩa trong task_webserver.cpp
extern AsyncWebServer server;
extern AsyncWebSocket ws;

// Khởi tạo toàn bộ route + WebSocket + OTA
void connnectWSV();

// Vòng lặp duy trì + OTA loop
void Webserver_reconnect();

// Dừng server
void Webserver_stop();

// Gửi broadcast qua WS
void Webserver_sendata(String data);

// RTOS entry: WebServer chạy ở AP mode
void Task_Webserver(void *pvParameters);

// (tùy chọn) prototype từ task_handler.cpp — giữ để tương thích dự án
void handleWebSocketMessage(String message);
// Hàm cũ (không còn dùng)
// void Webserver_reconnect();

#endif