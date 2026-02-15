#include "ChetchUtils.h"
#include "ChetchOLEDTextDisplay.h"
#include "ChetchArduinoBoard.h"


namespace Chetch{
    OLEDTextDisplay::OLEDTextDisplay(TextSize textSize, RefreshRate refreshRate) : 
        DisplayDevice(&oled, refreshRate),
#if defined(OLED_128x32_I2C)
      oled(/* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA)
#elif defined(OLED_128x64_I2C)
      oled (/* reset=*/ U8X8_PIN_NONE)
#else //default 128x32_I2C
      oled(/* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA)
#endif
    {

        this->textSize = textSize;
    }

	bool OLEDTextDisplay::begin(){
        if(oled.begin()){
            oled.setPowerSave(0);
            setFontSize(textSize);
            begun = true;
        } else {
            begun = false;
        }
        return begun;
	}
    
    void OLEDTextDisplay::loop(){
        DisplayDevice::loop();
    }

    
    void OLEDTextDisplay::clearDisplay(){
        if(isLocked())return;
        oled.clearDisplay();
    }

    void OLEDTextDisplay::setFontSize(TextSize textSize){
        switch(textSize){
            case LARGE_TEXT:
                oled.setFont(u8x8_font_7x14_1x2_f); 
                break;
            case SMALL_TEXT:
                oled.setFont(u8x8_font_chroma48medium8_r); 
                break;
            case XLARGE_TEXT:
                oled.setFont(u8x8_font_px437wyse700a_2x2_r);
                break;
        }
    }
}
