#ifndef CHETCH_ARDUINO_OLED2_H
#define CHETCH_ARDUINO_OLED2_H

#include <Arduino.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>

#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/displays/ChetchDisplayDevice.h"

//#define I2C_ADDRESS 0x3C
#if defined(ARDUINO_AVR_MEGA2560)
    #define DEFAULT_I2C_ADDRESS 0
#elif defined(ARDUINO_AVR_NANO)
    #define DEFAULT_I2C_ADDRESS 0x3C //other common addresses are: 0x3F
#elif defined(ARDUINO_AVR_UNO)
    #define DEFAULT_I2C_ADDRESS 0x3C //other common addresses are: 0x3F
#else
    #define DEFAULT_I2C_ADDRESS 0
#endif

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
    class OLEDTextDisplay2 : public DisplayDevice<SSD1306AsciiAvrI2c*>{
        public:
            

        private:
            SSD1306AsciiAvrI2c oled;
            uint8_t* font = System5x7;
            
        public:
            OLEDTextDisplay2(RefreshRate refreshRate = RefreshRate::REFRESH_10HZ);
            OLEDTextDisplay2(const uint8_t* font, RefreshRate refreshRate = RefreshRate::REFRESH_10HZ);

            void setFont(const uint8_t* font){ oled.setFont(font); };

            bool begin() override;
            void loop() override;
            //bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void clearDisplay() override;
                        
    };
} //end namespace
#endif
