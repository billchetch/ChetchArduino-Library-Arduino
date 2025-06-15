#include <Arduino.h>
#include <Wire.h>
#include <U8x8lib.h>

#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

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
            void setReportInfo(ArduinoMessage* message) override;
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void displayPreset(DisplayPreset preset, unsigned int lockFor = 3000);
            void displayBoardStats(unsigned int lockFor = 3000);
            void lock(unsigned int lockFor); //set to 0 to remove lock, lockFor in ms
            void unlock();
            bool isLocked(){ return lockDuration > 0; };
            U8X8* getDisplay(){ return isLocked() ? NULL : display; };
    };
} //end namespace