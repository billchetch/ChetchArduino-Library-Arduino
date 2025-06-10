#ifndef CHETCH_ARDUINO_TICKER_H
#define CHETCH_ARDUINO_TICKER_H

#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    class Ticker : public ArduinoDevice {
        public:
            static const byte EVENT_TICKED = 1;

        private:
            byte pin = 0;
            unsigned int pinHighDuration = 0;
            unsigned long pinHighStartedOn = 0;
            unsigned int pinLowDuration = 0;
            unsigned long pinLowStartedOn;
            unsigned long tickCount = 0;
            bool pinState = LOW;
            
        public: 
            
            Ticker();
            Ticker(byte pin, unsigned int highDuration, unsigned int lowDuration);
            void setHighLowDuration(unsigned int highDuration, unsigned int lowDuration);

            void setReportInfo(ArduinoMessage* message) override; 
            void loop() override;

            unsigned long getTickCount(){ return tickCount; };
    }; //end class
} //end namespae
#endif