#include "ChetchTDSMeter.h"

namespace Chetch{

    TDSMeter::TDSMeter(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize) : 
                    AnalogSampler(analogPin, sampleInterval, sampleSize)
    {

    }

    void TDSMeter::onSamplingComplete(){
        AnalogSampler::onSamplingComplete();
        
        double val = getValue();
    }
}