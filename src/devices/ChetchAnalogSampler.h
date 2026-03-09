#ifndef CHETCH_ANALOG_SAMPLER_H
#define CHETCH_ANALOG_SAMPLER_H

#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>
#include "ChetchADC.h"

namespace Chetch{
    class AnalogSampler : public ArduinoDevice {
        public:
            struct Results
            {
                double meanValue = 0.0;
                uint16_t minValue = 0;
                uint16_t maxValue = 0;
            };
            
            //function parameters: this, last reading, summed readings, number of readings
            typedef void (*SamplingCompleteListener)(AnalogSampler*); 

        private:
            byte analogPin = 0;
            CADC::AnalogReference aref;

            uint16_t sampleCount = 0;
            unsigned int sampleInterval = 0;
            unsigned int waitInterval = 0; //wait a period of time before starting sampling again
            unsigned long lastSampledOn = 0;
            unsigned long startedSamplingOn = 0;
            bool sampling = false;

            Results results;

            SamplingCompleteListener samplingCompleteListener = NULL;

        protected:
            uint16_t sampleSize = 0;
            unsigned long summedSamples = 0;

        public:
            uint16_t firstValue = 0;
            uint16_t lastValue = 0;
            uint16_t minValue = 0;
            uint16_t maxValue = 0;
            double meanValue = 0.0;
            uint16_t samplingDuration = 0;
            

        public:
            AnalogSampler(byte analogPin, unsigned int sampleInterval, uint16_t sampleSize = 1, uint16_t waitInterval = 0, CADC::AnalogReference aref = CADC::AnalogReference::AREF_INTERNAL);
            ~AnalogSampler();

            void setWaitInterval(unsigned int waitInteval);
            void addSamplingCompleteListener(SamplingCompleteListener listener){ samplingCompleteListener = listener; }

            void setStatusInfo(ArduinoMessage* message) override;
            void setReportInfo(ArduinoMessage* message) override;

            bool begin() override;
            void loop() override;

            double getVoltage(double val);

            bool isSamplingComplete();
            virtual void onSamplingComplete();
    }; //End class
} //End namespace
#endif
