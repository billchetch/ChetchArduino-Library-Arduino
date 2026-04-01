#include "ChetchAnalogSampler.h"

namespace Chetch{
    AnalogSampler::AnalogSampler(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize, uint16_t waitInterval, CADC::AnalogReference aref){
        this->analogPin = analogPin;
        this->sampleSize = sampleSize;
        //samples = new uint16_t[sampleSize == 0 ? 1 : sampleSize];
        this->sampleInterval = sampleInterval;
        setWaitInterval(waitInterval);

        if(aref != CADC::aref()){
            CADC::init(aref);
        }
        this->aref = aref;
    }

    AnalogSampler::~AnalogSampler(){
        //delete[] samples;
    }

    void AnalogSampler::setWaitInterval(unsigned int waitInterval){
        if(waitInterval < sampleInterval){
            this->waitInterval = sampleInterval;
        } else {
            this->waitInterval = waitInterval;
        }
    }

    bool AnalogSampler::begin(){
        pinMode(analogPin, INPUT);

        begun = true;
        return begun;
    }

    void AnalogSampler::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        
        message->add(sampleSize);
        message->add(sampleInterval);        
    }

    void AnalogSampler::setReportInfo(ArduinoMessage* message){
        ArduinoDevice::setReportInfo(message);

        //add results (only mean value for now)
        message->add(results.meanValue);
    }

    void AnalogSampler::loop(){
        ArduinoDevice::loop();

        unsigned int interval = sampling ? sampleInterval : waitInterval;
        if(millis() - lastSampledOn > interval){
            if(!sampling){
                CADC::startRead(analogPin);
                startedSamplingOn = millis();
                sampling = true;
            } else if(!CADC::isReading()){
                lastSampledOn = millis();
                lastValue = CADC::readResult();
                if(sampleCount == 0){
                    firstValue = lastValue;
                    minValue = firstValue;
                    maxValue = firstValue;
                }
                if(lastValue > maxValue)maxValue = lastValue;
                if(lastValue < minValue)minValue = lastValue;

                summedSamples += lastValue;
                sampleCount++;
                if(isSamplingComplete()){
                    sampling = false;
                    samplingDuration = millis() - startedSamplingOn;
                    meanValue = (double)summedSamples / (double)sampleCount;

                    //trigger listener (if overriden the overriding method should call base)
                    onSamplingComplete();

                    //reset
                    summedSamples = 0;
                    sampleCount = 0;
                }
            }
        }
    } //end loop

    void AnalogSampler::onSamplingComplete(){
        //store results
        results.meanValue = meanValue;
        results.minValue = minValue;
        results.maxValue = maxValue;
        
        //Call a listener if there is one
        if(samplingCompleteListener != NULL){
            samplingCompleteListener(this);
        }
    }

    bool AnalogSampler::isSamplingComplete(){
        return sampleCount == sampleSize;
    }

    double AnalogSampler::getVoltage(double val){
        return CADC::getVoltage(val, aref);
    }
}