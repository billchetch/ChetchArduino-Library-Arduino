#include "ChetchUtils.h"
#include "ChetchADC.h"
#include "ChetchZMPT101B.h"

#if defined(ARDUINO_AVR_MEGA2560)
    #define TIMER_NUMBER 4
    #define TIMER_PRESCALER 8 //'ticks'every 0.5 microseconds
#else
    #define TIMER_NUMBER 0
    #define TIMER_PRESCALER 0
#endif


namespace Chetch{
    
    ISRTimer* ZMPT101B::timer = NULL;
    byte ZMPT101B::instanceCount = 0;
    byte ZMPT101B::currentInstance = 0; //current instance for reading ISR
    ZMPT101B* ZMPT101B::instances[ZMPT101B::MAX_INSTANCES];
    
    ZMPT101B* ZMPT101B::create(byte id, byte cat, char *dn){
        if(instanceCount >= MAX_INSTANCES || TIMER_NUMBER <= 0){
            return NULL;
        } else {
            if(instanceCount == 0){
                cli();
                timer = ISRTimer::create(TIMER_NUMBER, TIMER_PRESCALER, ISRTimer::TimerMode::COMPARE);
                sei();

                CADC::init(false); //turn off trigger interrupt

                for(byte i = 0; i < MAX_INSTANCES; i++){
                    instances[i] = NULL;
                }
                currentInstance = 0;
            }

            //get first available slot
            byte idx = 0;
            for(byte i = 0; i < MAX_INSTANCES; i++){
                if(instances[i] == NULL){
                    idx = i;
                    break;
                }
            }

            ZMPT101B* instance = new ZMPT101B(id, cat, dn);
            instance->setInstanceIndex(idx);
            instance->useTimer = true;
            instances[idx] = instance;
            instanceCount++;

            unsigned int m = 500;
            unsigned int comp = timer->microsToTicks(m);
            /*Serial.print("Setting ZMPT101B timer comp to ");
            Serial.print(comp);
            Serial.print(" ticks = ");
            Serial.print(m);
            Serial.println(" micros");*/
            timer->registerCallback(&ZMPT101B::handleTimerInterrupt, ISRTimer::LOWEST_PRIORITY, comp);
            return instance;
        }
    }

    void ZMPT101B::handleTimerInterrupt(){
        static ZMPT101B* zmpt = NULL;
        
        //free up other interrupts
        //sei();

        zmpt = instances[currentInstance];
        
        if (!CADC::isReading()) {
            zmpt->onAnalogRead(CADC::readResult());
            CADC::startRead(zmpt->voltagePin);
        }
        
    }


    ZMPT101B::ZMPT101B(byte id, byte cat, char *dn) : ArduinoDevice(id, cat, dn){
        
        //clean to begin
        for(byte i = 0; i < BUFFER_SIZE; i++){
            buffer[i] = 0;
        }
    }

    ZMPT101B::~ZMPT101B(){
        instances[instanceIndex] = NULL;
        instanceCount--;
        if(timer->isEnabled()){
            timer->disable();
        }
    }

    void ZMPT101B::setInstanceIndex(byte idx){
        instanceIndex = idx;
    }

    void ZMPT101B::setVoltagePin(byte pin){
        voltagePin = pin;
        pinMode(voltagePin, INPUT);
    }

    bool ZMPT101B::configure(ADMMessage* message, ADMMessage* response){
        if(!ArduinoDevice::configure(message, response))return false;

        int argIdx = getArgumentIndex(message, MessageField::PIN);
        setVoltagePin( message->argumentAsByte(argIdx));

        argIdx = getArgumentIndex(message, MessageField::SAMPLE_SIZE);
        sampleSize = message->argumentAsInt(argIdx);

        target = (Target)message->argumentAsByte(getArgumentIndex(message, MessageField::TARGET));
        if(target != Target::NONE){
            setTargetParameters(
                target,
                message->argumentAsInt(getArgumentIndex(message, MessageField::TARGET_VALUE)),
                message->argumentAsInt(getArgumentIndex(message, MessageField::TARGET_TOLERANCE)),
                message->argumentAsInt(getArgumentIndex(message, MessageField::TARGET_LOWER_BOUND)),
                message->argumentAsInt(getArgumentIndex(message, MessageField::TARGET_UPPER_BOUND))
                );
        }

        
        response->addByte(target);
        response->addDouble(targetValue);

        return true;
    }

    void ZMPT101B::status(ADMMessage *message, ADMMessage *response){
        ArduinoDevice::status(message, response);

        response->addByte(voltagePin);
    }

    int ZMPT101B::getArgumentIndex(ADMMessage *message, ZMPT101B::MessageField field){
        switch(field){
            default:
                return (int)field;
        }
    }

    void ZMPT101B::populateMessageToSend(byte messageID, ADMMessage* message){
        ArduinoDevice::populateMessageToSend(messageID, message);

        if(messageID == ArduinoDevice::MESSAGE_ID_REPORT){
            message->addDouble(getVoltage());
            message->addDouble(getHz());
            message->addULong(val1);
            message->addULong(val2);
            message->addULong(val3);
            message->addULong(val4);
            message->addULong(val5);
        }

        if(messageID == MESSAGE_ID_ADJUSTMENT){
            populateMessage(ADMMessage::MessageType::TYPE_WARNING, message);
            message->addDouble(adjustBy());
        }
    }

	void ZMPT101B::setTargetParameters(Target t, double tv, double tt, double tlb, double tub){
        target = t;
        targetValue = tv;
        targetTolerance = tt;
        targetLowerBound = tlb;
        targetUpperBound = tub;
    }                   
    
    void ZMPT101B::setHzThresholdVoltage(int threshold) {
        hzThresholdVoltage = threshold;
    }

    void ZMPT101B::onAnalogRead(uint16_t value) {
        //This method is called by the timer interrupt
        
        if (bufferIdx < BUFFER_SIZE) {
            buffer[bufferIdx++] = value;
        }
        
    }

    void ZMPT101B::loop(){
        ArduinoDevice::loop(); 
        
        if(useTimer && !timer->isEnabled() && isReady()){
            timer->enable();
            CADC::startRead(voltagePin);
            return;
        }

        if (samplingPaused)return;

        //Per sample set values
        static unsigned long sampleCount = 0;
        static unsigned long summedVoltages = 0;
        static unsigned long hzCount = 0;
        static unsigned long hzCountDuration = 0; //in timer ticks
        //static int tempBuffer[BUFFER_SIZE];
        //static bool temp = false;
        
        if (bufferIdx >= BUFFER_SIZE) {
            //per batch values
            int prevVoltage = 0;
            int hzCountStartedOn = -1;
            int hzCountEndedOn = 0;
            int currentPosition = 0; //1 means above 10v, -1 means below 10v, 0 unknown
            int prevPosition = 0; //1 means above 10v, -1 means below 10v, 0 unknown
            
            //go through the data on teh buffer
            for (int i = 0; i < bufferIdx; i++) {
                int currentVoltage = (int)buffer[i] - midPoint;
                //tempBuffer[i] = currentVoltage; // buffer[i];
                
                if (currentVoltage > hzThresholdVoltage) {
                    currentPosition = 1;
                }
                else if (currentVoltage < -hzThresholdVoltage) {
                    currentPosition = -1;
                }
                else {
                    currentPosition = 0;
                }

                //sum the squares for rms later
                summedVoltages += ((long)currentVoltage * (long)currentVoltage);

                if (currentPosition == 1 && prevPosition == -1 || currentPosition == -1 && prevPosition == 1) {
                    if (hzCountStartedOn == -1) {
                        hzCountStartedOn = i;
                    }
                    else {
                        hzCountEndedOn = i;
                        hzCount++;
                    }
                }
                if (currentPosition != 0)prevPosition = currentPosition;
            }

            hzCountDuration += hzCountEndedOn - hzCountStartedOn;
            sampleCount += BUFFER_SIZE;

            //fill up buffer again for some more samples
            cli();
            bufferIdx = 0;
            sei();
        } //finished processing buffer

        //now process all the samples
        if (sampleCount >= sampleSize) {
            //some analysis
            double newVoltage = sqrt((double)summedVoltages / (double)sampleCount); // *scaleWaveform) + finalOffset;
            uint32_t duration = timer->interruptsToMicros(&ZMPT101B::handleTimerInterrupt, hzCountDuration);
            double newHz = (500000.0 * (double)hzCount) / (double)duration;
            val1 = hzCountDuration;
            val2 = timer->getCompareA();
            val3 = timer->interruptCounts[1];
            val4 = hzCountDuration * 1 * val2 * timer->prescaler;
            val5 = timer->prescaler;
            
            assignResults(newVoltage, newHz);

            //reset for next set of samples
            sampleCount = 0;
            summedVoltages = 0;
            hzCount = 0;
            hzCountDuration = 0;
        } //end sample finished conditional
    }

    void ZMPT101B::pauseSampling(bool resetValues) {
        samplingPaused = true;
        if (resetValues) {
            voltage = 0;
            hz = 0;
            outOfRange = false;
            targetLost = false;
            targetReached = false;
        }
    }

    void ZMPT101B::resumeSampling(bool resetValues) {
        samplingPaused = false;
        if (resetValues) {
            voltage = 0;
            hz = 0;
            outOfRange = false;
            targetLost = false;
            targetReached = false;
        }
    }

    bool ZMPT101B::isSamplingPaused() {
        return samplingPaused;
    }

    ZMPT101B::Direction ZMPT101B::getDirection(double newVal, double oldVal, double tolerance) {
        double diff = newVal - oldVal;
        if (abs(diff) <= tolerance) {
            return Direction::Stable;
        }
        else if (diff > 0) {
            return Direction::Rising;
        }
        else {
            return Direction::Falling;
        }
    }

    void ZMPT101B::assignResults(double newVoltage, double newHz) {
        
        //do some rounding
        if(precisionFactor > 0){
            newVoltage = round(newVoltage * precisionFactor) / precisionFactor;
            newHz = round(newHz * precisionFactor) / precisionFactor;
        }

        //assess direction
        voltageDirection = getDirection(newVoltage, voltage, 0.0);
        hzDirection = getDirection(newHz, hz, 0.0);

        //assign to key properties
        voltage = newVoltage;
        hz = newHz;

        /*Serial.print(" V: ");
        Serial.print(voltage);
        Serial.print(",");
        Serial.print(" Hz: ");
        Serial.print(hz);
        Serial.println();*/

        //raise some events
        raiseEvent(EVENT_NEW_RESULTS);

        
        if (inTargetRange()) {
            outOfRange = false;
            if (adjustBy() != 0) {
                targetReached = false;
                if (!targetLost) { //raise event one time
                    targetLost = true;
                    raiseEvent(EVENT_TARGET_LOST);
                }
            }
            else if (!targetReached) {
                targetReached = true;
                targetLost = false;
                raiseEvent(EVENT_TARGET_REACHED);
            }
        }
        else if (!outOfRange){
            outOfRange = true;
            targetLost = false;
            targetReached = false;
            raiseEvent(EVENT_OUT_OF_TARGET_RANGE);
        }
    }

    double ZMPT101B::getVoltage(){
        return voltage;
    }

    double ZMPT101B::getHz(){
        return hz;
    }

    char *ZMPT101B::getSummary(){
        char strv[6];
        char strh[6];
        static char summary[16];
        dtostrf(getVoltage(), 3, 1, strv);
        dtostrf(getHz(), 3, 1, strh);
        sprintf(summary, "%sV %sHz", strv, strh);
        return summary;
    }

    double ZMPT101B::getCurrentValue(){
        switch(target){
            case Target::HZ:
                return getHz();
            
            case Target::VOLTAGE:
                return getVoltage();
            
            default:
                return -1;
        }
    }

    //whether the current value is even worth considering
    bool ZMPT101B::inTargetRange(){
        if(targetUpperBound <= targetLowerBound)return true;
      
        double v = getCurrentValue();
        return (v >= targetLowerBound && v <= targetUpperBound);
    }
    
    double ZMPT101B::adjustBy(){
        if(targetValue <= 0 || !inTargetRange())return 0;

        double v = getCurrentValue();
        double adjustment = targetValue - v;
        return abs(adjustment) <= targetTolerance ? 0 : adjustment;
    } 

    ZMPT101B::Direction ZMPT101B::getDesiredDirection() {
        return getDirection(adjustBy(), 0.0, 0.0);
    }

    ZMPT101B::Direction ZMPT101B::getCurrentDirection() {
        switch(target) {
            case Target::HZ:
                return hzDirection;

            case Target::VOLTAGE:
                return voltageDirection;

            default:
                return -1;
        }
    }

} //end namespace
