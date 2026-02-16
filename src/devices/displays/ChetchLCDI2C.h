#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/displays/ChetchDisplayDevice.h"

#if defined(ARDUINO_AVR_MEGA2560)
    #define DEFAULT_I2C_ADDRESS 0
#elif defined(ARDUINO_AVR_NANO)
    #define DEFAULT_I2C_ADDRESS 0x27 //other common addresses are: 0x3F
#elif defined(ARDUINO_AVR_UNO)
    #define DEFAULT_I2C_ADDRESS 0x27 //other common addresses are: 0x3F
#else
    #define I2C_ADDRESS 0
#endif


/*
PIN REFERENCE FOR I2C:
- VCC (5V)
- GND
- SDA = A4 (Nano/Arduino), 20 (Mega/Leonardo)
- SCL = A5 (Nano/Arduino), 21 (Mega/Leonardo)

Note: that for LCD displays many have the SCL and SDA pins reversed when compared to OLED i2c units
In short check the SDA and SCL pin order (VCC and GND are normally the same)

*/

namespace Chetch{
    class LCDI2C : public DisplayDevice<LiquidCrystal_I2C*>{
        public:
            
        private:
            LiquidCrystal_I2C lcd;

        public:
            LCDI2C(byte cols, byte rows, RefreshRate refreshRate = REFRESH_50Hz);
            
            bool begin() override;
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void clearDisplay() override;
    };
} //end namespace