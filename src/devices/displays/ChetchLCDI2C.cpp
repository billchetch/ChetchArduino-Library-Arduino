#include "ChetchUtils.h"
#include "ChetchArduinoBoard.h"
#include "ChetchLCDI2C.h"


namespace Chetch{
    LCDI2C::LCDI2C(byte cols, byte rows, RefreshRate refreshRate) : DisplayDevice(&lcd, rows, cols, refreshRate), lcd(DEFAULT_I2C_ADDRESS, cols, rows) {
        //empty
    }
    
    bool LCDI2C::begin(){
        lcd.init();
        lcd.backlight();
        //display.noBacklight();
        begun = true;
        return begun;
	}

    void LCDI2C::loop(){
        DisplayDevice::loop();

        //Do stuff here
    }

    bool LCDI2C::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = DisplayDevice::executeCommand(command, message, response);
        
        if(!handled)
        {
            
        }
        return handled;
    }

    void LCDI2C::clearDisplay(){
        lcd.clear();
    }

    //Takes control of screen for period in question then clears it after use
    
    /*void LCDI2C::clearLine(byte lineNumber){
        if(isLocked() || display == NULL)return;
        display->clearLine(lineNumber);
    }

    void LCDI2C::setFontSize(DisplayOption displayOption){
        switch(displayOption){
            case LARGE_TEXT:
                display->setFont(u8x8_font_7x14_1x2_f); 
                break;
            case SMALL_TEXT:
                display->setFont(u8x8_font_chroma48medium8_r); 
                break;
            case XLARGE_TEXT:
                display->setFont(u8x8_font_px437wyse700a_2x2_r);
                break;
        }
    }

    void LCDI2C::print(char* text, unsigned int cx, unsigned int cy){
        if(isLocked() || display == NULL)return;

        display->setCursor(cx, cy);
        display->print(text);
    }

    void LCDI2C::displayPreset(DisplayPreset preset, unsigned int lockFor){
         if(isLocked() || display == NULL)return;

        switch(preset){
            case BOARD_STATS:
                displayBoardStats(lockFor);
                break;

            case HELLO_WORLD:
                display->clearDisplay();
                display->setFont(u8x8_font_7x14_1x2_f);
                display->setCursor(0, 0);
                display->print("Hello World!");
                lock(lockFor);
                break;

            case CLEAR:
                display->clearDisplay();
                lock(lockFor);
                break;

            default:
                display->clearDisplay();
                display->setFont(u8x8_font_7x14_1x2_f);
                display->setCursor(0, 0);
                display->print("Preset not found");
                lock(lockFor);
                break;
        }
    }

    void LCDI2C::displayBoardStats(unsigned int lockFor){
        if(isLocked() || display == NULL)return;

        display->clearDisplay();
        display->setFont(u8x8_font_7x14_1x2_f);
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
        if(lockFor > 0){
            lock(lockFor);
        }
    }*/
}
