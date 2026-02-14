#include "ChetchUtils.h"
#include "ChetchSerialPin.h"


namespace Chetch{
    SerialPin::SerialPin(byte pin, int interval, byte frameSize)
    {
        this->pin = pin;
        this->interval = interval;

        this->frameSize = frameSize == 0 ? 1 : frameSize;
        frame = new byte[this->frameSize];
    }

    SerialPin::~SerialPin(){
        delete[] frame;
    }
    
    void SerialPin::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        
        message->add(pin);
        message->add(interval);
    }

    void SerialPin::loop(){
        ArduinoDevice::loop();

        if(!ready4comms && pinRead(false) == 1){
            ready4comms = true;
        }
    }

    bool SerialPin::intervalElapsed(byte slip){
        return (millis() - lastPinIO) > (interval + slip);
    }

    byte SerialPin::pinRead(bool asData){
        if(asData)lastPinIO = millis();
        return digitalRead(pin) & 0x01;
    }
} //end namespace
