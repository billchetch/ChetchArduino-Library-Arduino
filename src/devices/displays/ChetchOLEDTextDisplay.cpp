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
    
    void OLEDTextDisplay::loop(){
        ArduinoDevice::loop();

        if(isLocked() && millis() - lockedAt > lockDuration){
            if(display != NULL){
                display->clearDisplay();
            }
            unlock();
        }
    }

    bool OLEDTextDisplay::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = ArduinoDevice::executeCommand(command, message, response);
        
        if(!handled)
        {
            char text[32];
            switch(command){
                case DIZPLAY:
                    displayPreset((DisplayPreset)message->get<DisplayPreset>(1),
                                    message->get<unsigned int>(2));
                    handled = true;
                    break;

                case PRINT:
                    if(message->getArgumentSize(1) < 32){
                        message->get(1, text);
                        print(text, message->get<unsigned int>(2), message->get<unsigned int>(3));
                    } else {
                        //todo an error of sorts
                    }
                    handled = true;
                    break;

                case LOCK:
                    //todo
                    break;

                default:
                    handled = false;
                    break;

            }
        }
        return handled;
    }
    
    //Takes control of screen for period in question then clears it after use
    void OLEDTextDisplay::lock(unsigned int lockFor){
        lockDuration = lockFor;
        lockedAt = millis();
    }

    void OLEDTextDisplay::unlock(){
        lockDuration = 0;
    }

    void OLEDTextDisplay::clearDisplay(){
        if(isLocked() || display == NULL)return;
        display->clearDisplay();
    }

    void OLEDTextDisplay::print(char* text, unsigned int cx, unsigned int cy){
        if(isLocked() || display == NULL)return;

        display->setFont(u8x8_font_7x14_1x2_f);
        display->setCursor(cx, cy);
        display->print(text);
    }

    void OLEDTextDisplay::displayPreset(DisplayPreset preset, unsigned int lockFor){
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

    void OLEDTextDisplay::displayBoardStats(unsigned int lockFor){
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
    }
}
