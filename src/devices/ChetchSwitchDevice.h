#ifndef CHETCH_ADM_SWITCH_DEVICE_H
#define CHETCH_ADM_SWITCH_DEVICE_H

#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    class SwitchDevice : public ArduinoDevice {
        public:
            enum SwitchMode {
                ACTIVE = 1,
                PASSIVE = 2
            };

            static const byte MESSAGE_ID_TRIGGERED = 100;
            static const int EVENT_SWITCH_TRIGGERED = 1;

            static const byte ERROR_SWITCH_MODE = 101;

        private:
            SwitchMode mode;
            byte pin = 0;
            bool pinState = false;
            int tolerance = 0;
            unsigned long recording = 0;
            bool on = false;
            
        public: 
            
            SwitchDevice();
            SwitchDevice(SwitchMode mode, byte pin, int tolerance, bool pinState = LOW);

            void configure(SwitchMode mode, byte pin, int tolerance, bool pinState = LOW);
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;
            virtual void trigger();
            bool isOn();

            void setStatusInfo(ArduinoMessage* message) override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;

    }; //end class
} //end namespae
#endif