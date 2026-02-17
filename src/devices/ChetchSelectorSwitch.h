#ifndef CHETCH_ARDUINO_SELECTOR_SWITCH_H
#define CHETCH_ARDUINO_SELECTOR_SWITCHH


#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/ChetchSwitchDevice.h"

namespace Chetch{
    class SelectorSwitch : public SwitchDevice {
        public:
            typedef void (*SelectListener)(SelectorSwitch*, byte); //this, the pin number of the selected pin

        private:
            byte firstPin = 0;
            byte lastPin = 0;
            byte selectedPin = 0;
            
            unsigned long lastChecked = 0;

            SelectListener selectListener = NULL;

        public:
            SelectorSwitch(SwitchDevice::SwitchMode mode, byte firstPin, byte maxPins, int tolerance = 100, bool onState = LOW);
            
            void addSelectListener(SelectListener listener){ selectListener = listener; }

            void setStatusInfo(ArduinoMessage* message) override;
            bool begin() override;
            void loop() override;
            
            void trigger() override;

            byte getSelectedPin(){ return selectedPin; }
    }; //end class
} //end namespae
#endif