#ifndef CHETCH_TDS_METER_H
#define CHETCH_TDS_METER_H

#include "devices/ChetchAnalogSampler.h"

namespace Chetch{
    class TDSMeter : public AnalogSampler {
        public:
            //Cubic curve fit coefficients (taken from online example: https://randomnerdtutorials.com/arduino-tds-water-quality-sensor/)
            static constexpr double cA = 133.42*0.5; 
            static constexpr double cB = -255.86*0.5;
            static constexpr double cC = 857.39*0.5;
        
            struct TDSResults{
                double ppm = 0.0;
                double voltage = 0.0;
            };

        private:
            double temperature = 25.0;
            double ppm = 0.0;
            unsigned int ppmMin = 0;
            unsigned int ppmMax = 1000;

            TDSResults tdsResults;
            
        public:
            TDSMeter(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize = 1);

            void setRange(unsigned int min, unsigned int max){ ppmMin = min; ppmMax = max; }
            void setTemperature(double t){ temperature = t; }
            double getTemperature(){ return temperature; }
            double getPPM(){ return ppm; }
            void onSamplingComplete() override;

            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;

    }; //end class
} //end namespace
#endif