#ifndef CHETCH_CAN_BUS_BASE_H
#define CHETCH_CAN_BUS_BASE_H

#include "ChetchArduinoBoard.h"
#include "devices/comms/can/ChetchMCP2515Device.h"
#include "devices/comms/serial/ChetchSerialPin.h"

#define NODE_PRESENCE_INTERVAL 5000
#define SERIAL_PIN_INTERVAL 50 
#define SERIAL_PIN_BUFFER_SIZE 2

namespace Chetch{
    class CANBusBase : public ArduinoBoard{
        private:
            MCP2515Device* pmcp;
            SerialPin* pspin;

        public:
            CANBusBase(MCP2515Device* pmcp, SerialPin* pspin);

            MCP2515Device* getMCP(){ return pmcp; }

            byte getNodeID(){ return pmcp->getNodeID(); }
    };
}
#endif