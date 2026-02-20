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

    void OLEDTextDisplay2::initialiseDisplay(){
        if(begun){
            Wire.end();
            delay(1);
        }
        
        Wire.begin();
        Wire.setClock(400000L);
        Wire.setWireTimeout(25000, false);

        if(begun){
            delay(1);
        }
        oled.setFont(font);
        oled.begin(&Adafruit128x32, DEFAULT_I2C_ADDRESS);
        if(begun){
            delay(1);
        }
    }

    bool OLEDTextDisplay2::isDisplayConnected(){
        if(Wire.getWireTimeoutFlag() != 0){
            Wire.clearWireTimeoutFlag();
        }
        Wire.beginTransmission(DEFAULT_I2C_ADDRESS); // Start transmission to the I2C addres
        bool transmitSuccess = Wire.endTransmission() == 0;
        return transmitSuccess && (Wire.getWireTimeoutFlag() == 0);
    }

    /*void OLEDTextDisplay2::loop(){
        DisplayDevice::loop();
    }*/

    void OLEDTextDisplay2::clearDisplay(){
        if(isLocked())return;
        oled.clear();
        setCursor(0, 0);
    }

    
}
