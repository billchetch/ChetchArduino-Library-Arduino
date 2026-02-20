#include <Arduino.h>
#include <Wire.h>
#include <U8x8lib.h>

#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/displays/ChetchDisplayDevice.h"

/*
PIN REFERENCE FOR I2C:
- VCC (5V)
- GND
- SDA = A4 (Nano/Arduino), 20 (Mega/Leonardo)
- SCL = A5 (Nano/Arduino), 21 (Mega/Leonardo)

Note: that for OLED displays many have the SCL and SDA pins reversed when compared to LCD i2c units
In short check the SDA and SCL pin order (VCC and GND are normally the same)
*/

namespace Chetch{
    class OLEDTextDisplay : public DisplayDevice<U8X8*>{
        public:
            enum TextSize{
                LARGE_TEXT,
                SMALL_TEXT,
                XLARGE_TEXT
            };

        private:
            #if defined(OLED_128x32_I2C)
                U8X8_SSD1306_128X32_UNIVISION_HW_I2C oled;
            #elif defined(OLED_128x64_I2C)
                U8X8_SSD1306_128X64_NONAME_HW_I2C oled;
            #else //default
                U8X8_SSD1306_128X32_UNIVISION_HW_I2C oled;
            #endif

            TextSize textSize;
            
            
        public:
            OLEDTextDisplay(TextSize textSize = TextSize::LARGE_TEXT, RefreshRate refreshRate = RefreshRate::REFRESH_10HZ);
            
            bool begin() override;
            void loop() override;
            //bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void initialiseDisplay() override {}
            bool isDisplayConnected() override { return true; } //TODO: properly!!!
            void clearDisplay() override;
            void setFontSize(TextSize textSize);
                        
    };
} //end namespace