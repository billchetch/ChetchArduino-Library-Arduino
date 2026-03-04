#include "ChetchSelectorSwitch.h"

namespace Chetch{
    
    SelectorSwitch::SelectorSwitch(SwitchDevice::SwitchMode mode, byte firstPin, byte selectionSize, int tolerance, bool onState) : SwitchDevice(mode, firstPin, tolerance, onState){
        this->firstPin = firstPin;
        if(selectionSize < 1)selectionSize = 2;
        this->lastPin = firstPin + (selectionSize - 1);
    }
   

    bool SelectorSwitch::begin(){
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

        if(selectedPin == 0 && millis() - lastChecked > getTolerance() + 10UL){
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
        //Base adds reportInterval, mode, onstate and pinstate = 4 args = 5 bytes
        SwitchDevice::setStatusInfo(message);
        
        //we now add pin range and selected pin
        message->add((byte)firstPin);
        message->add((byte)lastPin);
        message->add(getSelectedPin()); //total = 7 args 8 bytes
    }

    void SelectorSwitch::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        SwitchDevice::populateOutboundMessage(message, messageID);
        
        if(messageID == MESSAGE_ID_TRIGGERED){
            message->add(getSelectedPin());             
        }
    }
    
    void SelectorSwitch::trigger(){
        SwitchDevice::trigger();

        selectedPin = isOn() ? getPin() : 0;
        if(selectedPin > 0 && selectListener != NULL){
            selectListener(this, selectedPin);
        }
    }
}