#include "ChetchTDSMeter.h"

namespace Chetch{

    TDSMeter::TDSMeter(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize, uint16_t waitInterval) : 
                    AnalogSampler(analogPin, sampleInterval, sampleSize, waitInterval)
    {

    }

    void TDSMeter::onSamplingComplete(){
        //top and tail mean value
        double clippedMean = meanValue;
        if(sampleSize >= 3){
            unsigned long clippedSum = summedSamples - minValue - maxValue;
            clippedMean = (double)clippedSum / (double)(sampleSize - 2);
        }

        //temperature adjusted voltage
        double v = getVoltage(clippedMean);
        double x = v/(1.0 + 0.02*(temperature - 25.0));

        //cubic curve fit
        double newppm = cA*x*x*x + cB*x*x + cC*x;
        ppm = (newppm + ppm) / 2.0; //some smoothing
        tdsResults.lowerBound = false;
        tdsResults.upperBound = false;
        if(ppm < ppmMin){
            ppm = (double)ppmMin;
            tdsResults.lowerBound = true;
        }
        tdsResults.upperBound = false;
        if(ppm > ppmMax){
            ppm = (double)ppmMax;
            tdsResults.upperBound = true;
        }

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