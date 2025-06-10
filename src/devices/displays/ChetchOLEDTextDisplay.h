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

        public:
            U8X8* display = NULL;
            uint8_t *defaultFont;
            DisplayOption displayOption;

        public:
            OLEDTextDisplay(DisplayOption displayOption = DisplayOption::LARGE_TEXT);
            ~OLEDTextDisplay();

            bool begin() override;

            void displayBoardStats(unsigned int showFor = 3000);
    };
} //end namespace