#include "ChetchAnalogSampler.h"

namespace Chetch{
    AnalogSampler::AnalogSampler(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize, CADC::AnalogReference aref){
        this->analogPin = analogPin;
        this->sampleSize = sampleSize;
        //samples = new uint16_t[sampleSize == 0 ? 1 : sampleSize];
        this->sampleInterval = sampleInterval;

        if(aref != CADC::aref()){
            CADC::init(aref);
        }
        this->aref = aref;
    }

    AnalogSampler::~AnalogSampler(){
        //delete[] samples;
    }

    bool AnalogSampler::begin(){
        pinMode(analogPin, INPUT);

        begun = true;
        return begun;
    }

    void AnalogSampler::loop(){
        if(millis() - lastSampledOn > sampleInterval){
            lastSampledOn = millis();
            if(!sampling){
                CADC::startRead(analogPin);
                startedSamplingOn = millis();
                sampling = true;
            } else if(!CADC::isReading()){
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