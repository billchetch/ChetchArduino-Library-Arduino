#ifndef CHETCH_CAN_BUS_BASE_H
#define CHETCH_CAN_BUS_BASE_H

#include "ChetchArduinoBoard.h"
#include "devices/comms/ChetchMCP2515Device.h"
#include "devices/comms/ChetchSerialPin.h"

#define NODE_PRESENCE_INTERVAL 10000
#define SERIAL_PIN_INTERVAL 50 
#define SERIAL_PIN_BUFFER_SIZE 2

namespace Chetch{
    class CANBusBase : public ArduinoBoard{
        private:
            MCP2515Device* pmcp;
            SerialPin* pspin;

        public:
            CANBusBase(MCP2515Device* pmcp, SerialPin* pspin);

            virtual void handleReceivedMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData);
    };
}
#endif