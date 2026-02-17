#ifndef CHETCH_ADM_SWITCH_DEVICE_H
#define CHETCH_ADM_SWITCH_DEVICE_H

#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    class SwitchDevice : public ArduinoDevice {
        public:
            enum SwitchMode : byte {
                ACTIVE = 1,
                PASSIVE = 2,
            };

            static const byte MESSAGE_ID_TRIGGERED = 100;
            static const byte EVENT_SWITCH_TRIGGERED = 8;

            static const byte ERROR_SWITCH_MODE = 101;

        private:
            SwitchMode mode;
            byte pin = 0;
            bool pinState = LOW;
            bool onState;
            int tolerance = 0;
            unsigned long recording = 0;

        protected:
            void initPin(byte pin);
            void setPin(byte pin); //allow pin change (good for derived functionality)
            byte getPin(){ return pin; };
            int getTolerance(){ return tolerance; }
            SwitchMode getMode(){ return mode; }

        public: 
            
            SwitchDevice();
            SwitchDevice(SwitchMode mode, byte pin, int tolerance, bool onState);

            bool begin() override;
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;
            virtual void trigger();
            void turn(bool on);
            bool isOn();

            void setStatusInfo(ArduinoMessage* message) override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;

    }; //end class
} //end namespae
#endif