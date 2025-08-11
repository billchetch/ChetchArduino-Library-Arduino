#include "ChetchUtils.h"
#include "ChetchCounter.h"
#include "ChetchInterrupt.h"


namespace Chetch{
    byte Counter::instanceCount = 0;
    Counter* Counter::instances[];
    volatile unsigned long Counter::handleInterruptCount = 0;

    void Counter::handleInterrupt(uint8_t pin, uint8_t tag) {
        //handleInterruptCount++;
        if(instanceCount > tag){
            instances[tag]->onInterrupt();
        }
    }

    bool Counter::addInstance(Counter* instance) {
        if (instanceCount >= MAX_INSTANCES) {
            return false;
        } else {
            if (instanceCount == 0) {
                for (byte i = 0; i < MAX_INSTANCES; i++) {
                    instances[i] = NULL;
                }
            }
            byte idx = 0;
            for (byte i = 0; i < MAX_INSTANCES; i++) {
                if(instances[i] == NULL){
                    idx = i;
                    break;
                }
            }
            instance->setInstanceIndex(idx);
            instances[idx] = instance;
            instanceCount++;
            return true;
        }
    }

    Counter::Counter(byte pin, byte interruptMode, unsigned long tolerance, bool pinStateToCount) : ArduinoDevice(){
        setPin(pin);

        setInterruptMode(interruptMode);
        
        this->tolerance = tolerance; 
        this->pinStateToCount = pinStateToCount;

        addInstance(this);
    }

    Counter::~Counter() {
        if (interruptMode != 0) {
            CInterrupt::removeInterruptListener(pin);
        }
        instances[instanceIndex] = NULL;
        instanceCount--;
    }

    void Counter::setInstanceIndex(byte idx){
        instanceIndex = idx;
    }

    void Counter::setPin(byte pin){
        this->pin = pin;
        pinMode(this->pin, INPUT); 
        
        bitMask = digitalPinToBitMask(pin);
        inreg = portInputRegister(digitalPinToPort(pin));
        pinState = digitalRead(pin);
    }

    bool Counter::setInterruptMode(byte mode){
        if (mode != 0 && interruptMode == 0) { //one time set
            interruptMode = mode;
            return CInterrupt::addInterruptListener(pin, instanceIndex, handleInterrupt, interruptMode);
        }
        return true;
    }

    void Counter::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        ArduinoDevice::populateOutboundMessage(message, messageID);

        /*if(messageID == ArduinoDevice::MESSAGE_ID_REPORT){
            //setting this to false means the interrupt handler won't assign values to the vars below (single byte no cli/sei needed)
            countStarted = false;

            //assign to 
            message->addULong(count);
            unsigned long duration = micros() - countStartedOn;
            message->addULong(duration);
            duration = count > 1 ? lastCountOn - firstCountOn : 0;
            message->addULong(duration);
        }*/
    }


    void Counter::loop(){
        ArduinoDevice::loop(); 
        
        if(!countStarted){
            count = 0;
            countStartedOn = micros();
            firstCountOn = 0;
            lastCountOn = 0;

            //make sure this is at the end .. this way we don't need cli/sei
            countStarted = true;
        }

        if(interruptMode == 0 && ((bitMask & *inreg) == bitMask) != pinState){ //not using interrupts
            onInterrupt();
        } //end no interrupt condition
    }

    void Counter::onInterrupt(){
        if(countStarted){
            unsigned long mcs = micros();
            if(count > 0 && mcs - countedOn < tolerance)return;

            pinState  = (bitMask & *inreg) == bitMask;
            if(pinState == pinStateToCount){
                countedOn = mcs;
                if(count == 0){
                    firstCountOn = countedOn;
                } else {
                    lastCountOn = countedOn;
                }
                count++;
            }
            
        }
    }

    unsigned long Counter::getCount(){
        return count;
    }

    
} //end namespace
