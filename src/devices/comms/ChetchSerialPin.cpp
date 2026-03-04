#include "ChetchUtils.h"
#include "ChetchSerialPin.h"


namespace Chetch{
    SerialPin::SerialPin(byte pin, int interval, byte bufferSize)
    {
        this->pin = pin;
        this->interval = interval;

        this->bufferSize = bufferSize == 0 ? 1 : bufferSize;
        buffer = new byte[this->bufferSize];
    }

    SerialPin::~SerialPin(){
        delete[] buffer;
    }
    
    void SerialPin::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        
        message->add(pin);
        message->add(interval);
        message->add(bufferSize);
    }

    void SerialPin::loop(){
        ArduinoDevice::loop();

        if(!ready4comms && pinRead(false) == 1){
            ready4comms = true;
        }
    }

    bool SerialPin::intervalElapsed(byte slip){
        return (millis() - lastPinIO) > (unsigned long)(interval + slip);
    }

    byte SerialPin::pinRead(bool asData){
        if(asData)lastPinIO = millis();
        return digitalRead(pin) & 0x01;
    }
} //end namespace
