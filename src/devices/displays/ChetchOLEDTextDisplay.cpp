#include "ChetchUtils.h"
#include "ChetchOLEDTextDisplay.h"
#include "ChetchArduinoBoard.h"
#include <MemoryFree.h>


namespace Chetch{
    OLEDTextDisplay::OLEDTextDisplay(DisplayOption displayOption){
#if defined(OLED_128x32_I2C)
      display = new U8X8_SSD1306_128X32_UNIVISION_HW_I2C(/* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);
#elif defined(OLED_128x64_I2C)
      display = new U8X8_SSD1306_128X64_NONAME_HW_I2C (/* reset=*/ U8X8_PIN_NONE); 
#else //default
      display = new U8X8_SSD1306_128X32_UNIVISION_HW_I2C(/* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);
#endif

        this->displayOption = displayOption;

        switch(displayOption){
            case LARGE_TEXT:
                defaultFont = u8x8_font_7x14_1x2_f; 
                break;
            case SMALL_TEXT:
                defaultFont = u8x8_font_chroma48medium8_r; 
                break;
            case XLARGE_TEXT:
                defaultFont = u8x8_font_px437wyse700a_2x2_r;
                break;
        }
    }

    OLEDTextDisplay::~OLEDTextDisplay(){
        if(display != NULL)delete display;
    } 
    
	bool OLEDTextDisplay::begin(){
        if(display != NULL && display->begin()){
            display->setPowerSave(0);
            display->setFont(defaultFont);
        } else {
            return false;
        }
	}

    void OLEDTextDisplay::displayBoardStats(unsigned int showFor){
        display->setFont(u8x8_font_7x14_1x2_f);
        display->clearDisplay();
        display->setCursor(0, 0);
        display->print(BOARD_NAME);
        display->print(" ");
        display->print(freeMemory());
        display->print(" bytes");
        display->setFont(u8x8_font_chroma48medium8_r);
        display->setCursor(0, 2);
        display->print("Devices: ");
        display->print(Board->getDeviceCount());
        display->setCursor(0, 3);
        display->print("Display: ");
        display->print(display->getCols());
        display->print("x");
        display->print(display->getRows());
        if(showFor > 0){
            delay(showFor);
            display->clearDisplay();
        }
        display->setFont(defaultFont);
    }
}
