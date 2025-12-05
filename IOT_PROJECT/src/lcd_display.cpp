#include "global.h"
#include "lcd_display.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h> 

// Địa chỉ I2C, 16 cột, 2 hàng
LiquidCrystal_I2C lcd(33, 16, 2); 

void lcd_display_task(void *pvParameters){
    Serial.println("LCD Task: Running..");
    
    lcd.begin();
    lcd.backlight();

    LcdState_t localState;
    float localTemp = 0.0; 
    float localHumi = 0.0; 

    while(1){
       
        if (xSemaphoreTake(xLcdStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            localState = g_currentLcdState;
            xSemaphoreGive(xLcdStateMutex);
        }

    
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            localTemp = glob_temperature; 
            localHumi = glob_humidity;   
            xSemaphoreGive(xDataMutex);
        }

        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            lcd.clear();
            lcd.setCursor(0, 0);

         
            lcd.print("T:"); 
            lcd.print(localTemp, 1); 
            lcd.print((char)223); 
            lcd.print("C ");     

            lcd.print("H:"); 
            lcd.print(localHumi, 1); 
            lcd.print("%");      
        

            lcd.setCursor(0, 1);
            switch (localState) {
                case LCD_NORMAL:
                    lcd.print("  BINH THUONG  ");
                    break;
                case LCD_WARNING:
                    lcd.print("!! CANH BAO !!");
                    break;
                case LCD_CRITICAL:
                    lcd.print("! NGUY HIEM !");
                    break;
            }
            xSemaphoreGive(xI2CMutex);
            
        } else {
             Serial.println("[LCD Task] ERROR: xI2CMutex timeout");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}