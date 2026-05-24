#ifndef CHETCH_MCP2515_MONITOR_H
#define CHETCH_MCP2515_MONITOR_H

#include <Arduino.h>
#include <limits.h>

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
    class MCP2515Monitor : public MCP2515Device{
        public:            
            static const byte MESSAGE_ID_FORWARD_RECEIVED = 100;
            static const byte MESSAGE_ID_FORWARD_SENT = 101;
            static const int DEFAULT_CS_PIN = 6; //To free up pins 8 and 9 for AltSoftSerial

            static const unsigned int FORWARD_TIMEOUT = 20000; //compared to last status request
            static const byte EVENT_FORWARDING_SET = 16;

            class NodeData{
                public:
                    byte nodeID = 0;
                    byte issues = 0;
                    
                    unsigned int presenceCount = 0; //on rollover this goes straight to 1
                    unsigned int statusResponseCount = 0;
                    unsigned int messageCount = 0;
                    NodeData* nextNode = NULL;

                public:
                    NodeData(byte nodeID){
                        this->nodeID = nodeID;
                    }

                    void update(ArduinoMessage* message){
                        unsigned int n;
                        bool expectedValue = true;
                        switch(message->type){
                            case ArduinoMessage::TYPE_PRESENCE:
                                n = message->get<unsigned int>(2);    
                                if(presenceCount != 0){
                                    if(presenceCount == UINT_MAX){
                                        expectedValue = n == 1;
                                    } else {
                                        expectedValue = n == (presenceCount + 1);
                                    }
                                    if(!expectedValue)issues = issues | 0x01;
                                } 
                                presenceCount = n;
                                break;

                            case ArduinoMessage::TYPE_STATUS_RESPONSE:
                                statusResponseCount++;
                                break;
                        }
                        messageCount++;
                    }
            };
            
            typedef void (*ForwardingListener)(MCP2515Monitor*, ArduinoMessage*); 

        private:
            ArduinoMessage frecvmsg;
            ArduinoMessage fsendmsg;

            unsigned int messageCount = 0;

            bool canForward = false;
            ForwardingListener forwardingListener;

            unsigned long lastStatusRequest = 0;
            bool statusRequested = false;

            NodeData* firstNodeData = NULL;
            byte nodeCount = 0;
            NodeData* node2report = NULL;
            
        public:
            MCP2515Monitor(byte nodeID, int csPin = DEFAULT_CS_PIN, unsigned int presenceInterval = 0);
            ~MCP2515Monitor();

            void loop() override;

            bool canForwardMessages(){ return canForward; }
            void addForwardingListener(ForwardingListener listener){ forwardingListener = listener; }
            NodeData* getNodeData(byte nodeID, bool createIfNotFound);
            
            void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response) override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;

            void setReportInfo(ArduinoMessage* message) override;
            void setStatusInfo(ArduinoMessage* response) override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void onMessageReceived(byte sourceNodeID, ArduinoMessage *message) override;
            void onMessageSent(ArduinoMessage *message) override;
    };
} //end namespace
#endif //end prevent multiple inclusion