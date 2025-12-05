#ifndef __TASK_WIFI_H__
#define __TASK_WIFI_H__

#include <Arduino.h> // Thêm thư viện cơ sở

// Hàm này được gọi bởi main.cpp (chế độ Client)
void Task_Wifi(void *pvParameters);

// Hàm này được gọi bởi task_webserver.cpp (chế độ AP)
void startAP();

// Các hàm cũ (không còn dùng trong logic RTOS)
// bool Wifi_reconnect();
// void startSTA();

#endif