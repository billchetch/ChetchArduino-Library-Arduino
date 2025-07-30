#include <Arduino.h>
#include <Wire.h>
#include <U8x8lib.h>

#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>


/*
PIN REFERENCE FOR I2C LED:
- 5V
- GND
- SDA = A4 (Nano/Arduino), 20 (Mega/Leonardo)
- SCL = A5 (Nano/Arduino), 21 (Mega/Leonardo)
*/

namespace Chetch{
    class OLEDTextDisplay : public ArduinoDevice{
        public:
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

        private:
            U8X8* display = NULL;
            uint8_t *defaultFont;
            DisplayOption displayOption;
            unsigned int lockDuration = 0;
            unsigned long lockedAt = 0;

        public:
            OLEDTextDisplay(DisplayOption displayOption = DisplayOption::LARGE_TEXT);
            ~OLEDTextDisplay();

            bool begin() override;
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void clearDisplay();
            void print(char* text, unsigned int cx = 0, unsigned int cy = 0);
            void displayPreset(DisplayPreset preset, unsigned int lockFor);
            void displayBoardStats(unsigned int lockFor);
            void lock(unsigned int lockFor); //set to 0 to remove lock, lockFor in ms
            void unlock();
            bool isLocked(){ return lockDuration > 0; };
            U8X8* getDisplay(){ return isLocked() ? NULL : display; };
    };
} //end namespace