#ifndef CHETCH_CAN_BUS_MASTER_H
#define CHETCH_CAN_BUS_MASTER_H

#include "ChetchCANBusBase.h"
#include "devices/comms/ChetchMCP2515Monitor.h"
#include "devices/comms/ChetchSerialPinMaster.h"

/*
A can bus master is a 'monitor' board but with a serial pin master to control entire bus
*/

namespace Chetch{

    class CANBusMaster : public CANBusBase{
        public:
            static const byte MASTER_NODE_ID = 1;
            
        protected:
            MCP2515Monitor mcp;
            SerialPinMaster spin;
            ArduinoIO io;

        public:
            CANBusMaster(byte serialPin);

            bool begin(Stream *stream);
    };
} //end namespace
#endif
