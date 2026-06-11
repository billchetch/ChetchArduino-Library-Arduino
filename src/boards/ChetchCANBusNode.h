#ifndef CHETCH_CAN_BUS_NODE_H
#define CHETCH_CAN_BUS_NODE_H

#include "ChetchCANBusBase.h"
#include "ChetchCANBusIO.h"
#include "devices/comms/can/ChetchMCP2515Device.h"
#include "devices/comms/serial/ChetchSerialPinSlave.h"

namespace Chetch{

    class CANBusNode : public CANBusBase{
        public:
            static const int DEFAULT_CS_PIN = 10;

        protected:
            MCP2515Device mcp;
            SerialPinSlave spin;
            CANBusIO io;

        private:
            //REMVOE! for debug only this
            unsigned int statusRequestCount = 0;
            unsigned int statusResponseCount = 0;
            

        public:
            CANBusNode(byte nodeID, byte serialPin);

            bool begin(MessageIO* io = NULL) override; //will return false if fails to begin

            virtual void handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData);
            virtual bool sendBusMessageValidator(ArduinoMessage* message, byte* canData);
            
            void setStatusInfo(ArduinoMessage* message) override;
            void setReportInfo(ArduinoMessage* message) override;
    };
} //end namespace
#endif
