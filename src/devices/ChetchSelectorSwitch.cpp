#include "ChetchSelectorSwitch.h"

namespace Chetch{
    
    SelectorSwitch::SelectorSwitch(SwitchDevice::SwitchMode mode, byte firstPin, byte lastPin, int tolerance, bool onState) : SwitchDevice(mode, firstPin, tolerance, onState){
        this->firstPin = firstPin > lastPin ? lastPin : firstPin;
        this->lastPin = lastPin < firstPin ? firstPin : lastPin;
    }
   

    bool SelectorSwitch::begin(){
        if(firstPin == lastPin){
            begun = false;
            return false;
        }

        if(SwitchDevice::begin()){
            //Initialise the physical pins (other than the first one which is handled above by switch begin)
            for(byte i = firstPin + 1; i <= lastPin; i++){
                initPin(i);
            }
            begun = true;
            return begun;
        } else {
            return false;
        }
    }

    void SelectorSwitch::loop(){

        if(selectedPin == 0 && millis() - lastChecked > getTolerance() + 10){
            if(!isOn()){
                byte nextPin = getPin() + 1;
                if(nextPin > lastPin)nextPin = firstPin;
                setPin(nextPin);
            }
            lastChecked = millis();
        }

        SwitchDevice::loop();
    }

    void SelectorSwitch::setStatusInfo(ArduinoMessage* message){
        SwitchDevice::setStatusInfo(message);
        
        message->add(lastPin);
        message->add(selectedPin);
    }

    
    void SelectorSwitch::trigger(){
        SwitchDevice::trigger();

        selectedPin = isOn() ? getPin() : 0;
        if(selectedPin > 0 && selectListener != NULL){
            selectListener(this, selectedPin);
        }
    }
}