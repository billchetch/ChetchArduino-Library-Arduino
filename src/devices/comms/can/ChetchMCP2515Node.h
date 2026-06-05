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
            static const int DEFAULT_CS_PIN = 10;
            
        private:
            
        public:
            MCP2515Node(byte nodeID = 0, int csPin = MCP2515Node::DEFAULT_CS_PIN, unsigned int presenceInterval = MCP2515Device::DEFAULT_PRESENCE_INTERVAL);
            void setNodeID(byte nodeID){ this->nodeID = nodeID; }

            bool begin() override;
            //void loop() override;
            void setReportInfo(ArduinoMessage* message) override;
            
    };
} //end namespace
#endif //end prevent multiple inclusion