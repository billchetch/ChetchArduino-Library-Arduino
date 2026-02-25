#ifndef CHETCH_CAN_BUS_NODE_H
#define CHETCH_CAN_BUS_NODE_H

#include "ChetchCANBusBase.h"
#include "devices/comms/ChetchMCP2515Node.h"
#include "devices/comms/ChetchSerialPinSlave.h"



namespace Chetch{

    class CANBusNode : public CANBusBase{
        public:
            
        protected:
            MCP2515Node mcp;
            SerialPinSlave spin;

        public:
            CANBusNode(byte nodeID, byte serialPin);

    };
} //end namespace
#endif
