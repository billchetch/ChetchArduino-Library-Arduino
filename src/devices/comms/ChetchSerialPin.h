#ifndef CHETCH_SERIALPIN_DEVICE_H
#define CHETCH_SERIALPIN_DEVICE_H

#include <Arduino.h>

#include <ChetchArduinoBoard.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    class SerialPin : public ArduinoDevice{
        public:
            byte pin = 0;
                        
        public:
            SerialPin(byte pin);
        
            bool begin() override;
            void loop() override;
    }; //end class

} //end namespace

#endif //end prevent multiple inclusions