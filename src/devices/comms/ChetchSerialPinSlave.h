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
            
            
        protected:
            DataListener dataListener = NULL;

        public:
            SerialPinSlave(byte pin, int interval = 100, byte frameSize = 1) : SerialPin(pin, interval, frameSize){}
            
            void addDataListener(DataListener listener){ dataListener = listener; }

            bool begin() override;
            void loop() override;

            template<typename T> T get(byte* bytes = NULL){
                if(bytes == NULL)bytes = frame;
                T retVal = 0;
                for(byte i = 0; i < sizeof(T); i++){
                    ((byte *)&retVal)[i] = bytes[i];
                }
                return retVal;
            }
    }; //end class

} //end namespace

#endif //end prevent multiple inclusions