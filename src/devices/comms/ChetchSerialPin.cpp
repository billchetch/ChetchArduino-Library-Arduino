#include "ChetchUtils.h"
#include "ChetchSerialPin.h"


namespace Chetch{
    SerialPin::SerialPin(byte pin)
    {
        this->pin = pin;
    }
    
    
    bool SerialPin::begin(){
        begun = true;
        return begun;
	}


    void SerialPin::loop(){
        indicate(false);
        ArduinoDevice::loop();
    }
} //end namespace
