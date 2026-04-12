#ifndef CHETCH_ARDUINO_MESSAGE_HANDLER_H
#define CHETCH_ARDUINO_MESSAGE_HANDLER_H

#include <Arduino.h>

#include "ChetchArduinoMessage.h"

namespace Chetch{
    class ArduinoMessageHandler{
        private:
            byte id;

        public:
            virtual void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response) = 0;
            virtual void populateOutboundMessage(ArduinoMessage* message, byte messageID) = 0;
        
            byte getID(){ return id; }
            void setID(byte id){ this->id = id; }
    }; //end class
}//end namesapce
#endif