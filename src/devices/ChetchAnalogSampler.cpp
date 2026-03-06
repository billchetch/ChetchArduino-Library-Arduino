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
                uint16_t result = CADC::readResult();
                summedSamples += result;
                sampleCount++;
                if(isSamplingComplete()){
                    sampling = false;
                    
                    //trigger listener
                    if(samplingCompleteListener != NULL){
                        samplingCompleteListener(this, result, summedSamples, sampleCount);
                    }

                    //reset
                    summedSamples = 0;
                    sampleCount = 0;
                }
            }
        }
    } //end loop

    bool AnalogSampler::isSamplingComplete(){
        return sampleCount == sampleSize;
    }

    double AnalogSampler::getAverageReading(){
        if(isSamplingComplete()){
            return (double)summedSamples / (double)sampleSize;
        } else {
            return -1.0;
        }
    }
}