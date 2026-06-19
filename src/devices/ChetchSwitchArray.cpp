#include "ChetchSwitchArray.h"

namespace Chetch{
    
    SwitchArray::SwitchArray(SwitchDevice::SwitchMode mode, byte firstPin, byte arraySize, int tolerance, bool onState) : SwitchDevice(mode, firstPin, tolerance, onState){
        this->firstPin = firstPin;
        if(arraySize < 1)arraySize = 2;
        if(arraySize > 8)arraySize = 8;
        this->lastPin = firstPin + (arraySize - 1);
        pin2check = this->firstPin;
    }


    bool SwitchArray::begin(){
        //TODO: remove this and add functinoality for an active switch array
        if(getMode() == SwitchDevice::SwitchMode::ACTIVE){
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

    void SwitchArray::loop(){

        if(millis() - lastChecked > getTolerance() + 10UL){
            pin2check++;
            if(pin2check > lastPin)pin2check = firstPin;

            bool pState = digitalRead(pin2check);
            bool on = pState == getOnState();

            //check if this current pin is in a different state from the last time we set it
            if(on != isSwitchOn(pin2check)){
                //Serial.print("State chang so pin is: ");
                //Serial.println(pin2check);
                //set the current pin to the pin we are focusing on and set pin state to opposite
                //so that the SwitchDevice::loop method picks up a difference and starts recording
                setPin(pin2check);
                setPinState(!pState); //set 
            }

            lastChecked = millis();
        }

        SwitchDevice::loop();
    }

    void SwitchArray::setStatusInfo(ArduinoMessage* message){
        //Base adds reportInterval, mode, onstate and pinstate = 4 args = 5 bytes
        SwitchDevice::setStatusInfo(message);
        
        //we now add pin range and on flags
        message->add((byte)firstPin);
        message->add((byte)lastPin);
        message->add((byte)onFlags);
        
    }

    void SwitchArray::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        SwitchDevice::populateOutboundMessage(message, messageID);
        
        if(messageID == MESSAGE_ID_TRIGGERED){
            message->add(onFlags);             
        }
    }
    
    bool SwitchArray::isSwitchOn(byte pinNumber){
        byte flagPosition = pinNumber - getFirstPin();
        byte mask = 1 << flagPosition;
        return (mask & onFlags) == mask;
    }

    void SwitchArray::setOnFlag(byte flagPosition, bool on){
        byte mask = 1 << flagPosition;
        if(on){
            onFlags = onFlags | mask;
        } else {
            onFlags = onFlags & ~mask;
        }
    }

    void SwitchArray::trigger(){
        
        byte bitPosition = getPin() - getFirstPin();
        setOnFlag(bitPosition, isOn());

        SwitchDevice::trigger();        
    }
}