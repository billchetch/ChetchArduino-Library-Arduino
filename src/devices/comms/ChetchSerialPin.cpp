#include "ChetchUtils.h"
#include "ChetchSerialPin.h"


namespace Chetch{
    SerialPin::SerialPin(byte pin, int interval)
    {
        this->pin = pin;
        this->interval = interval;
    }
    
    void SerialPin::loop(){
        ArduinoDevice::loop();

        if(!ready4comms && pinRead() == 1){
            ready4comms = true;
        }
    }

    bool SerialPin::intervalElapsed(byte slip){
        return (millis() - lastPinIO) > (interval + slip);
    }

    byte SerialPin::pinRead(){
        lastPinIO = millis();
        return digitalRead(pin) & 0x01;
    }
} //end namespace
