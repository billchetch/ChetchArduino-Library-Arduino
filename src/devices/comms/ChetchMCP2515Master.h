#ifndef CHETCH_MCP2515_MASTER_H
#define CHETCH_MCP2515_MASTER_H

#include <Arduino.h>

#include <ChetchArduinoBoard.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "ChetchMCP2515Device.h"

/*
Concept of Master is that it is an connection between the CAN bus and the Arduino Board messaging
structure (namely a Strem object ... normall serial connected by USB to a computer).

A CAN bus could be made of just nodes.  Only one master is allowed and has fixed Node ID = 1.
See base class for full info
*/

namespace Chetch{
    class MCP2515Master : public MCP2515Device{
        public:            
            static const byte MESSAGE_ID_FORWARD_RECEIVED = 100;
            static const byte MESSAGE_ID_FORWARD_SENT = 101;
            static const byte MESSAGE_ID_READY_TO_SEND = 102;
            
        private:
            ArduinoMessage frecvmsg;
            ArduinoMessage fsendmsg;
            
        public:
            MCP2515Master(int csPin = CAN_DEFAULT_CS_PIN);

            bool begin() override;
            bool allowSending() override;
            void raiseError(MCP2515ErrorCode errorCode, unsigned int errorData) override;

            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            void setStatusInfo(ArduinoMessage* response) override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            bool sendMessage(ArduinoMessage *message) override;
            void handleReceivedMessage(byte sourceNodeID, ArduinoMessage *message) override;
    };
} //end namespace
#endif //end prevent multiple inclusion