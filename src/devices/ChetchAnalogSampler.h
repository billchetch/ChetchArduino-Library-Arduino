#ifndef CHETCH_ANALOG_SAMPLER_H
#define CHETCH_ANALOG_SAMPLER_H

#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>
#include "ChetchADC.h"

namespace Chetch{
    class AnalogSampler : public ArduinoDevice {
        public:
            //function parameters: this, last reading, summed readings, number of readings
            typedef void (*SamplingCompleteListener)(AnalogSampler*, uint16_t, unsigned long, uint16_t); 

        private:
            byte analogPin = 0;
            uint16_t sampleSize = 0;
            uint16_t sampleCount = 0;
            unsigned long summedSamples = 0;
            unsigned int sampleInterval = 0;
            unsigned long lastSampledOn = 0;
            bool sampling = false;
            
            SamplingCompleteListener samplingCompleteListener = NULL;

        public:
            AnalogSampler(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize = 1, CADC::AnalogReference aref = CADC::AnalogReference::AREF_INTERNAL);
            ~AnalogSampler();

            void addSamplingCompleteListener(SamplingCompleteListener listener){ samplingCompleteListener = listener; }
            
            bool begin() override;
            void loop() override;

            bool isSamplingComplete();
            double getAverageReading();
    }; //End class
} //End namespace
#endif
