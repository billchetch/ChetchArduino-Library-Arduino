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
                sampling = true;
            } else if(!CADC::isReading()){
                lastRead = CADC::readResult();
                summedSamples += lastRead;
                sampleCount++;
                if(isSamplingComplete()){
                    sampling = false;
                    
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
            samplingCompleteListener(this, lastRead, summedSamples, sampleCount);
        }
    }

    bool AnalogSampler::isSamplingComplete(){
        return sampleCount == sampleSize;
    }

    double AnalogSampler::getValue(){
        if(isSamplingComplete()){
            return (double)summedSamples / (double)sampleSize;
        } else {
            return -1.0;
        }
    }
}