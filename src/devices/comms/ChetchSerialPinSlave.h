#ifndef CHETCH_SERIALPIN_SLAVE_DEVICE_H
#define CHETCH_SERIALPIN_SLAVE_DEVICE_H

#include <Arduino.h>

#include "ChetchSerialPin.h"

namespace Chetch{
    class SerialPinSlave : public SerialPin {
        public:
            static const byte EVENT_BYTE_RECEIVED = 1;

            static const byte MESSAGE_ID_SEND_DATA = 100;

            typedef void (*DataListener)(SerialPinSlave*, byte*, byte); //device, buffer

        private: 
            byte* data2send;
            
        protected:
            DataListener dataListener = NULL;

        public:
            SerialPinSlave(byte pin, int interval = 100, byte bufferSize = 1);
            ~SerialPinSlave();

            void addDataListener(DataListener listener){ dataListener = listener; }

            bool begin() override;
            void loop() override;

            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;

            template<typename T> T get(byte* bytes = NULL){
                if(bytes == NULL)bytes = buffer;
                T retVal = 0;
                for(byte i = 0; i < sizeof(T); i++){
                    ((byte *)&retVal)[i] = bytes[i];
                }
                return retVal;
            }
    }; //end class

} //end namespace

#endif //end prevent multiple inclusions