#ifndef CHETCH_SERIALPIN_DEVICE_H
#define CHETCH_SERIALPIN_DEVICE_H

#include <Arduino.h>

#include <ChetchArduinoBoard.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    class SerialPin : public ArduinoDevice {

        public:
            
        protected: 
            byte pin = 0;

            unsigned long lastPinIO = 0;
            
            bool ready4comms = false;
            
            int interval = 100;
            byte data = 0;
            byte bitCount = 0;

        protected:
            byte pinRead();
            bool intervalElapsed(byte slip = 0);
            
        public:
            SerialPin(byte pin, int interval = 100);

            void loop() override;


    }; //end class

} //end namespace

#endif //end prevent multiple inclusions