#include "ChetchUtils.h"
#include "ChetchSwitchDevice.h"

namespace Chetch{
    
    SwitchDevice::SwitchDevice(){
        //empty
    }    
    SwitchDevice::SwitchDevice(SwitchMode mode, byte pin, int tolerance, bool onState){
        this->mode = mode;
        this->tolerance = tolerance;
        this->onState = onState;      
        setPin(pin);
    }   
    
    bool SwitchDevice::begin(){
        
        initPin(pin);

        begun = true;
        return begun;
    }

    void SwitchDevice::setPin(byte pin){
        this->pin = pin;
        recording = 0;
        pinState = !onState;  
    }

    void SwitchDevice::initPin(byte pin){
        switch (mode) {
        case SwitchMode::PASSIVE:
            if(onState == LOW){
                pinMode(pin, INPUT_PULLUP);
            } else {
                pinMode(pin, INPUT);
            }
            break;

        case SwitchMode::ACTIVE:
            pinMode(pin, OUTPUT);
            digitalWrite(pin, pinState);
            break;
        }
    }

    void SwitchDevice::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        
        message->add(mode);
        message->add(pinState);
    }

    void SwitchDevice::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        if(messageID == MESSAGE_ID_TRIGGERED){

            message->type = ArduinoMessage::TYPE_DATA;
            message->add(pinState);
            message->add(pin);             
        }
    }

	void SwitchDevice::loop(){
        ArduinoDevice::loop();
        
        if(mode == SwitchMode::PASSIVE){
            bool currentPinState = digitalRead(pin);
            if(currentPinState != pinState){
                //if there is a change of pin state then if we were already recording we simply reset
                //if we weren't recording then we start'
                recording = recording == 0 ? millis() : 0;
                pinState = currentPinState;
            } else if(recording > 0 && millis() - recording >= tolerance){ 
                //so here we are reording and have gone over tolerance so we trigger and reset
                trigger();
                recording = 0;
            }
        } else {
            if(recording > 0 && millis() - recording >= tolerance){
                trigger();
                recording = 0;
            }
        }

    }

    bool SwitchDevice::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = ArduinoDevice::executeCommand(command, message, response);
        
        if(!handled)
        {
            switch(command){
                case ON:
                case OFF:
                    if(mode == SwitchMode::ACTIVE){
                        turn(command == DeviceCommand::ON);
                        handled = true;
                    } else {
                        //add some error shit here 
                        setErrorInfo(response, ERROR_SWITCH_MODE);
                        handled = false;
                    }
                    break;

                default:
                    handled = false;
                    break;
            }
        }
                
        return handled;
    } 

    void SwitchDevice::trigger(){
        if(mode == SwitchMode::PASSIVE){
            enqueueMessageToSend(MESSAGE_ID_TRIGGERED);
        } else {
            enqueueMessageToSend(MESSAGE_ID_TRIGGERED);
            digitalWrite(pin, pinState);
        }
        raiseEvent(EVENT_SWITCH_TRIGGERED, isOn());
    }

    void SwitchDevice::turn(bool on){
        if(mode != SwitchMode::ACTIVE)return;

        pinState = on ? onState : !onState;
        bool currentPinState = digitalRead(pin);
        if(currentPinState != pinState){
            //if there is a request to change of pin state then if we haven't started recording then we start
            if(recording == 0)recording = millis();
            if(recording == 0)recording = 1; //in the highly unlikely case where millis == 0
        } else {
            //otherwise we've either requested something already the case OR we've undone our previous request
            recording = 0;
        }
    }

    
    bool SwitchDevice::isOn() {
        return pinState == onState;
    }
} //end namespace
