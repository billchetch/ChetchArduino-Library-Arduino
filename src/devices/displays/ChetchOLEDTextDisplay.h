#include <Arduino.h>
#include <Wire.h>
#include <U8x8lib.h>

#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>


/*
PIN REFERENCE FOR I2C OLED:
- 5V
- GND
- SDA = A4 (Nano/Arduino), 20 (Mega/Leonardo)
- SCL = A5 (Nano/Arduino), 21 (Mega/Leonardo)
*/

namespace Chetch{
    class OLEDTextDisplay : public ArduinoDevice{
        public:
            enum RefreshRate
            {
                REFRESH_1HZ = 1000,
                REFRESH_10HZ = 100,
                REFRESH_50Hz = 20
            };

            enum DisplayOption{
                LARGE_TEXT,
                SMALL_TEXT,
                XLARGE_TEXT
            };

            enum DisplayPreset{
                CLEAR = 0,
                BOARD_STATS,
                HELLO_WORLD
            };

            typedef bool (*DisplayHandler)(OLEDTextDisplay*, byte tag); 
        private:
            #if defined(OLED_128x32_I2C)
                U8X8_SSD1306_128X32_UNIVISION_HW_I2C display;
            #elif defined(OLED_128x64_I2C)
                U8X8_SSD1306_128X64_NONAME_HW_I2C display;
            #else //default
                U8X8_SSD1306_128X32_UNIVISION_HW_I2C display;
            #endif

            
            uint8_t *defaultFont;
            DisplayOption displayOption;
            unsigned int lockDuration = 0;
            unsigned long lockedAt = 0;
            
            DisplayHandler displayHandler = NULL;
            RefreshRate refreshRate = RefreshRate::REFRESH_50Hz;
            unsigned long lastUpdated = 0;
            bool update = false;
            byte updateTag = 0;
            
        public:
            OLEDTextDisplay(DisplayOption displayOption = DisplayOption::LARGE_TEXT, RefreshRate refreshRate = RefreshRate::REFRESH_10HZ);
            ~OLEDTextDisplay();

            bool begin() override;
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void updateDisplay(byte tag = 0);
            void addDisplayHandler(DisplayHandler handler){ displayHandler = handler; }
            void clearDisplay();
            void clearLine(byte lineNumber);
            void setFontSize(DisplayOption displayOption);
            void print(char* text, unsigned int cx = 0, unsigned int cy = 0);
            void displayPreset(DisplayPreset preset, unsigned int lockFor);
            void displayBoardStats(unsigned int lockFor);
            void lock(unsigned int lockFor); //set to 0 to remove lock, lockFor in ms
            void unlock();
            bool isLocked(){ return lockDuration > 0; };
            U8X8* getDisplay(){ return isLocked() ? NULL : &display; };
    };
} //end namespace