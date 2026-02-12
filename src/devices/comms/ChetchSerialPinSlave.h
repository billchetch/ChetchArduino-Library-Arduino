#ifndef CHETCH_SERIALPIN_SLAVE_DEVICE_H
#define CHETCH_SERIALPIN_SLAVE_DEVICE_H

#include <Arduino.h>

#include "ChetchSerialPin.h"

namespace Chetch{
    class SerialPinSlave : public SerialPin {
        public:
            static const byte EVENT_DATA_RECEIVED = 1;

        private: 
            
        protected:
            
        public:
            SerialPinSlave(byte pin, int interval = 100);

            bool begin() override;
            void loop() override;

            
    }; //end class

} //end namespace

#endif //end prevent multiple inclusions