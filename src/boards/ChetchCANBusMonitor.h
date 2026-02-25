#ifndef CHETCH_CAN_BUS_NODE_H
#define CHETCH_CAN_BUS_NODE_H

#include "ChetchCANBusBase.h"
#include "devices/comms/ChetchMCP2515Master.h"
#include "devices/comms/ChetchSerialPinMaster.h"


namespace Chetch{

    class CANBusMonitor : public CANBusBase{
        public:
            
        protected:
            MCP2515Master mcp;
            SerialPinMaster spin;

        public:
            CANBusMonitor(byte serialPin);

    };
} //end namespace
#endif
