#ifndef CHETCH_ARDUINO_SWITCH_ARRAY_H
#define CHETCH_ARDUINO_SWITCH_ARRAY_H


#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/ChetchSwitchDevice.h"

namespace Chetch{
    class SwitchArray : public SwitchDevice {
        public:
        
        private:
            byte onFlags = 0; //1 indicates ON (not necesarrily the pinstate)
            byte firstPin = 0;
            byte lastPin = 0;
            byte pin2check;
            
            unsigned long lastChecked = 0;
        
        
        protected: //TODO: revert to protected
            byte getFirstPin(){ return firstPin; }
            void setOnFlag(byte flagPosition, bool on);

            
        public:
            SwitchArray(SwitchDevice::SwitchMode mode, byte firstPin, byte arraySize, int tolerance = 50, bool onState = LOW);
            
            bool isSwitchOn(byte pinNumber);
            byte getOnFlags(){ return onFlags; }
            bool begin() override;
            void loop() override;

            void setStatusInfo(ArduinoMessage* message) override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            
            void trigger() override;

            
    }; //end class
} //end namespae
#endif