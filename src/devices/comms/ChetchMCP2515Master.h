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
            
        private:
            ArduinoMessage frecvmsg;
            ArduinoMessage fsendmsg;

            unsigned int messageCount = 0;

            bool canForward = false;
            unsigned long lastStatusRequest = 0;
            bool statusRequested = false;
            
        public:
            MCP2515Master(unsigned int presenceInterval = MCP2515Device::PRESENCE_INTERVAL, int csPin = CAN_DEFAULT_CS_PIN);

            bool begin() override;
            void loop() override;
            
            void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response) override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;

            void setReportInfo(ArduinoMessage* message) override;
            void setStatusInfo(ArduinoMessage* response) override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            
            void handleReceivedMessage(byte sourceNodeID, ArduinoMessage *message) override;
            bool sendMessage(ArduinoMessage *message) override;
    };
} //end namespace
#endif //end prevent multiple inclusion