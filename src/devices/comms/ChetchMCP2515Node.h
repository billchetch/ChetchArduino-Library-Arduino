#ifndef CHETCH_MCP2515_NODE_H
#define CHETCH_MCP2515_NODE_H

#include <Arduino.h>

#include <ChetchArduinoBoard.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>


#include "ChetchMCP2515Device.h"

/*
See base class for full info
*/

namespace Chetch{
    class MCP2515Node : public MCP2515Device{
        public:
            
        private:
            
        public:
            MCP2515Node(byte nodeID = 0, unsigned long presenceInterval = MCP2515Device::PRESENCE_INTERVAL, int csPin = CAN_DEFAULT_CS_PIN);
    };
} //end namespace
#endif //end prevent multiple inclusion