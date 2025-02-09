#include "ChetchUtils.h"
#include "ChetchSwitchDevice.h"

namespace Chetch{
    
    SwitchDevice::SwitchDevice(){
        //empty
    }    
    SwitchDevice::SwitchDevice(SwitchMode mode, byte pin, int tolerance, bool pinState){
        configure(mode, pin, tolerance, pinState);
    }    

    void SwitchDevice::configure(SwitchMode mode, byte pin, int tolerance, bool pinState){
        this->mode = mode;
        this->pin = pin;
        this->tolerance = tolerance;
        this->pinState = pinState;

        switch (mode) {
        case SwitchMode::PASSIVE:
            pinMode(pin, INPUT);
            break;

        case SwitchMode::ACTIVE:
            pinMode(pin, OUTPUT);
            digitalWrite(pin, pinState);
            break;
        }

        on = pinState == HIGH;
    }

    void SwitchDevice::setStatusInfo(ArduinoMessage* message){
        message->add(mode);
        message->add(pinState);
    }

    void SwitchDevice::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        if(messageID == MESSAGE_ID_TRIGGERED){

            message->type = ArduinoMessage::TYPE_DATA;
            message->add(pinState);             
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
                        pinState = command == DeviceCommand::ON;
                        bool currentPinState = digitalRead(pin);
                        if(currentPinState != pinState){
                            //if there is a request to change of pin state then if we haven't started recording then we start
                            if(recording == 0)recording = millis();
                        } else {
                            //otherwise we've either requested something already the case OR we've undone our previous request
                            recording = 0;
                        }
                        handled = true;
                    } else {
                        //add some error shit here 
                        setErrorInfo(response, ERROR_SWITCH_MODE);
                        handled = false;
                    }
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
        on = pinState == HIGH;
        raiseEvent(EVENT_SWITCH_TRIGGERED);
    }

    bool SwitchDevice::isOn() {
        return on;
    }
} //end namespace
