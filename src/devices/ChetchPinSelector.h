#ifndef CHETCH_ARDUINO_PIN_SELECTOR_H
#define CHETCH_ARDUINO_PIN_SELECTOR_H


#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/ChetchSwitchDevice.h"

namespace Chetch{
    class PinSelector : public SwitchDevice {
        public:
            typedef void (*SelectListener)(PinSelector*, byte); //this, the pin number of the selected pin

        private:
            byte firstPin = 0;
            byte lastPin = 0;
            byte selectedPin = 0;
            
            unsigned long lastChecked = 0;

            SelectListener selectListener = NULL;

        public:
            PinSelector(SwitchDevice::SwitchMode mode, byte firstPin, byte maxPins, int tolerance = 100, bool onState = LOW);
            
            void addSelectListener(SelectListener listener){ selectListener = listener; }

            void setStatusInfo(ArduinoMessage* message) override;
            bool begin() override;
            void loop() override;
            
            void trigger() override;

            byte getSelectedPin(){ return selectedPin; }
    }; //end class
} //end namespae
#endif