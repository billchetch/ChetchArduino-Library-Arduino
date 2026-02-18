#include "ChetchUtils.h"
#include "ChetchArduinoBoard.h"
#include "ChetchOLEDTextDisplay2.h"

namespace Chetch{
    OLEDTextDisplay2::OLEDTextDisplay2(const uint8_t* font, RefreshRate refreshRate) :  OLEDTextDisplay2(refreshRate)
    {
        this->font = font;
    }

    OLEDTextDisplay2::OLEDTextDisplay2(RefreshRate refreshRate) : 
        DisplayDevice(&oled, refreshRate)
        
    { }

	bool OLEDTextDisplay2::begin(){
        oled.begin(&Adafruit128x32, DEFAULT_I2C_ADDRESS);
        oled.setFont(font);
        
        begun = true;
        return begun;
	}
    
    void OLEDTextDisplay2::loop(){
        DisplayDevice::loop();
    }

    
    void OLEDTextDisplay2::clearDisplay(){
        if(isLocked())return;
        oled.clear();
        setCursor(0, 0);
    }

    
}
