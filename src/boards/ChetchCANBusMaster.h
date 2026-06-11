#ifndef CHETCH_CAN_BUS_MASTER_H
#define CHETCH_CAN_BUS_MASTER_H

#include "ChetchCANBusBase.h"
#include "ChetchArduinoIO.h"
#include "devices/comms/can/ChetchMCP2515Device.h"
#include "devices/comms/serial/ChetchSerialPinMaster.h"

/*
A can bus master is a 'monitor' board but with a serial pin master to control entire bus
*/

namespace Chetch{

    class CANBusMaster : public CANBusBase{
        public:
            static const byte MASTER_NODE_ID = 1;
            static const int DEFAULT_CS_PIN = 6; //To free up pins 8 and 9 for AltSoftSerial
            
        protected:
            MCP2515Device mcp;
            SerialPinMaster spin;
            ArduinoIO io;

        public:
            CANBusMaster(byte serialPin);

            bool begin(Stream *stream);
    };
} //end namespace
#endif
