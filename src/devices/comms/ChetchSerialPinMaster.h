#ifndef CHETCH_SERIALPIN_MASTER_DEVICE_H
#define CHETCH_SERIALPIN_MASTER_DEVICE_H

#include <Arduino.h>

#include "ChetchSerialPin.h"


namespace Chetch{
    class SerialPinMaster : public SerialPin {
        private: 
            bool sending = false;

        protected:
            void pinWrite(byte bit);
            
        public:
            SerialPinMaster(byte pin, int interval = 100, byte frameSize = 1) : SerialPin(pin, interval, frameSize){};

            bool begin() override;
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            bool send(byte *bytes, byte byteCount);

            bool send(byte b);
            bool send(int argv);
            bool send(long argv);
    }; //end class

} //end namespace

#endif //end prevent multiple inclusions