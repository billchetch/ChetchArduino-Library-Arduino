#include "ChetchUtils.h"
#include "ChetchCounter.h"
#include "ChetchInterrupt.h"


namespace Chetch{
    byte Counter::instanceCount = 0;
    Counter* Counter::instances[];
    
    void Counter::handleInterrupt(uint8_t pin, uint8_t tag) {
        //handleInterruptCount++;
        if(instanceCount > tag){
            instances[tag]->onInterrupt();
        }
    }

    int Counter::addInstance(Counter* instance) {
        if (instanceCount >= MAX_INSTANCES) {
            return -1;
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
            instances[idx] = instance;
            instanceCount++;
            return idx;
        }
    }

    Counter::Counter(byte pin, byte interruptMode, unsigned long assignValuesAfter, unsigned long tolerance, bool pinStateToCount) : ArduinoDevice(){
        //setPin(pin);
        this->pin = pin;
        this->interruptMode = interruptMode;
        this->assignValuesAfter = assignValuesAfter;
        this->pinStateToCount = pinStateToCount; //only relevant if interruptMode = 0

        this->tolerance = tolerance; 
        
        addInstance(this);
    }

    Counter::~Counter() {
        if (interruptMode != 0) {
            CInterrupt::removeInterruptListener(pin);
        }
        instances[instanceIndex] = NULL;
        instanceCount--;
    }

    bool Counter::begin(){
        //Set the pin
        pinMode(pin, INPUT);         
        bitMask = digitalPinToBitMask(pin);
        inreg = portInputRegister(digitalPinToPort(pin));
        pinState = digitalRead(pin);

        //if we are using interrupts then set accordingly here
        if(interruptMode != 0){
            //keep track of instances
            int idx = addInstance(this);
            if(idx < 0){
                return false;
            }
            instanceIndex = (byte)idx;
            if(!CInterrupt::addInterruptListener(pin, instanceIndex, &Counter::handleInterrupt, interruptMode)){
                return false;
            }
        }
        return true;
    }

    void Counter::assignValues(){
        //setting this to false means the interrupt handler won't assign values to the vars below (single byte no cli/sei needed)
        countStarted = false;
        lastCount = count;
        lastDuration = count > 1 ? lastCountOn - firstCountOn : 0;
        resetCount();
    }

    double Counter::getHz(){
        if(lastCount <= 1){
            return 0.0f;
        } else {
            return (double)(lastCount - 1) * (1000000.0 / (double)lastDuration);
        }
    }

    void Counter::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        ArduinoDevice::populateOutboundMessage(message, messageID);

        if(messageID == ArduinoDevice::MESSAGE_ID_REPORT){
            //assign to message
            message->add(lastCount);
            message->add(lastDuration);
            
        }
    }

    void Counter::loop(){
        ArduinoDevice::loop(); 
        
        if(!countStarted){
            resetCount();
        } else {
            if(assignValuesAfter > 0 && (millis() - countStartedOn > assignValuesAfter)){
                assignValues();
                raiseEvent(EVENT_ASSIGNED_VALUES);
            }
        }

        //in case we don't want to use an interrupt then we have this
        if(interruptMode == 0 && ((bitMask & *inreg) == bitMask) != pinState){ //not using interrupts
            unsigned long mcs = micros();
            if(tolerance > 0 && count > 0 && mcs - countedOn < tolerance){
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
        } //end no interrupt condition
    }

    void Counter::resetCount(){
        count = 0;
        countStartedOn = millis();
        firstCountOn = 0;
        lastCountOn = 0;

        countStarted = true;
    }

    void Counter::onInterrupt(){
        if(countStarted){
            unsigned long mcs = micros();
            if(tolerance > 0 && count > 0 && mcs - countedOn < tolerance)return;

            pinState  = (bitMask & *inreg) == bitMask; //if interrupt is set to CHANGING this will vary
            countedOn = mcs;
            if(count == 0){
                firstCountOn = countedOn;
            } else {
                lastCountOn = countedOn;
            }
            count++;
        }
    }

    unsigned long Counter::getCount(){
        return count;
    }

    
} //end namespace
