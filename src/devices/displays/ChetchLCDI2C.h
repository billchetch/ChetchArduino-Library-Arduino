#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>


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
- 5V
- GND
- SDA = A4 (Nano/Arduino), 20 (Mega/Leonardo)
- SCL = A5 (Nano/Arduino), 21 (Mega/Leonardo)
*/

namespace Chetch{
    class LCDI2C : public ArduinoDevice{
        public:
            /*enum DisplayOption{
                LARGE_TEXT,
                SMALL_TEXT,
                XLARGE_TEXT
            };*/

            enum DisplayPreset{
                CLEAR = 0,
                BOARD_STATS,
                HELLO_WORLD
            };

        private:
            LiquidCrystal_I2C display;

            //DisplayOption displayOption;
            unsigned int lockDuration = 0;
            unsigned long lockedAt = 0;

        public:
            LCDI2C(byte cols, byte rows);
            
            bool begin() override;
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void clearDisplay();
            /*void clearLine(byte lineNumber);
            void setFontSize(DisplayOption displayOption);
            void print(char* text, unsigned int cx = 0, unsigned int cy = 0);
            void displayPreset(DisplayPreset preset, unsigned int lockFor);
            void displayBoardStats(unsigned int lockFor);*/
            void lock(unsigned int lockFor); //set to 0 to remove lock, lockFor in ms
            void unlock();
            bool isLocked(){ return lockDuration > 0; };
            LiquidCrystal_I2C* getDisplay(){ return isLocked() ? NULL : &display; };
    };
} //end namespace