#include "ChetchTDSMeter.h"

namespace Chetch{

    TDSMeter::TDSMeter(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize, uint16_t waitInterval) : 
                    AnalogSampler(analogPin, sampleInterval, sampleSize, waitInterval)
    {

    }

    void TDSMeter::onSamplingComplete(){
        //temperature adjusted voltage
        double v = getVoltage(meanValue);
        double x = v/(1.0 + 0.02*(temperature - 25.0));

        //cubic curve fit
        ppm = cA*x*x*x + cB*x*x + cC*x;
        if(ppm < ppmMin)ppm = (double)ppmMin;
        if(ppm > ppmMax)ppm = (double)ppmMax;

        //store for message creation
        tdsResults.ppm = ppm;
        tdsResults.voltage = x;

        //Ensure any listeners are called
        AnalogSampler::onSamplingComplete();
    }

    void TDSMeter::setReportInfo(ArduinoMessage* message){
        AnalogSampler::setReportInfo(message);

        //add ppm
        message->add(tdsResults.ppm);
    }
}