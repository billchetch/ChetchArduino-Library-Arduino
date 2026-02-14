#ifndef CHETCH_SERIALPIN_SLAVE_DEVICE_H
#define CHETCH_SERIALPIN_SLAVE_DEVICE_H

#include <Arduino.h>

#include "ChetchSerialPin.h"

namespace Chetch{
    class SerialPinSlave : public SerialPin {
        public:
            static const byte EVENT_BYTE_RECEIVED = 1;

            typedef void (*DataListener)(SerialPinSlave*, byte*, byte); //device, buffer

        private: 
            byte* buffer;
            byte bufferLength = 0;
            byte bufferIdx = 0;
            
        protected:
            DataListener dataListener = NULL;

        public:
            SerialPinSlave(byte pin, int interval = 100, byte bufferLength = 1);
            ~SerialPinSlave();

            void addDataListener(DataListener listener){ dataListener = listener; }

            bool begin() override;
            void loop() override;

            
    }; //end class

} //end namespace

#endif //end prevent multiple inclusions