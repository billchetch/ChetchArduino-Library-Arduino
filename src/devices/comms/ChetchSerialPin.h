#ifndef CHETCH_SERIALPIN_DEVICE_H
#define CHETCH_SERIALPIN_DEVICE_H

#include <Arduino.h>

#include <ChetchArduinoBoard.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    class SerialPin : public ArduinoDevice {
        public:
            enum Role : byte{
                MASTER = 1,
                SLAVE = 2,
            };

        private: 
            Role role;
            byte pin = 0;

            unsigned long lastPinIO = 0;
            
            bool ready4comms = false;
            bool sending = false;

            int interval = 100;
            byte data = 0;
            byte bitCount = 0;

        protected:
            void pinWrite(byte bit);
            byte pinRead();

        public:
            SerialPin(Role role, byte pin);
        
            bool begin() override;
            void loop() override;

            bool send(byte b);
    }; //end class

} //end namespace

#endif //end prevent multiple inclusions