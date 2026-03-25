#ifndef CHETCH_CAN_BUS_MONITOR_H
#define CHETCH_CAN_BUS_MONITOR_H

#include "ChetchCANBusBase.h"
#include "devices/comms/ChetchMCP2515Monitor.h"
#include "devices/comms/ChetchSerialPinSlave.h"
#include "ChetchArduinoIO.h"

/*
A can bus monitor board uses the monitor mcp to wrap bus messages and route them to the stream
*/


namespace Chetch{

    class CANBusMonitor : public CANBusBase{
        public:
            
        protected:
            MCP2515Monitor mcp;
            SerialPinSlave spin;
            ArduinoIO io;

        public:
            CANBusMonitor(byte nodeID, byte serialPin);

            bool begin(Stream *stream);
    };
} //end namespace
#endif
