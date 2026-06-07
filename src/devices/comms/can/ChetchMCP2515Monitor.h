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
                    byte status = 0;
                    byte events = 0;
                    
                    unsigned int presenceCount = 0; //on rollover this goes straight to 1
                    unsigned int statusRequestCount = 0;
                    unsigned int statusResponseCount = 0;
                    unsigned int pingSentCount = 0;
                    unsigned int pingResponseCount = 0;
                    unsigned int messageReceivedCount = 0;
                    unsigned int messageSentCount = 0;
                    NodeData* nextNode = NULL;

                private:
                    void setStatusBit(byte bitPosition, bool val){
                        byte mask = 1 << bitPosition - 1;
                        if(val){
                            status = status | mask;
                            events = events | mask;
                        } else {
                            status = status & ~mask;
                        }
                    }

                public:
                    void clear(){
                        status = 0;
                        presenceCount = 0; //on rollover this goes straight to 1
                        statusRequestCount = 0;
                        statusResponseCount = 0;
                        pingSentCount = 0;
                        pingResponseCount = 0;
                        messageReceivedCount = 0;
                        messageSentCount = 0;
                        events = 0;
                    }

                
                    NodeData(byte nodeID){
                        this->nodeID = nodeID;
                    }

                    void received(ArduinoMessage* message){
                        unsigned int n;
                        bool expectedValue = true;
                        switch(message->type){
                            case ArduinoMessage::TYPE_ERROR:
                                setStatusBit(1, true);
                                break;

                            case ArduinoMessage::TYPE_PRESENCE:
                                n = message->get<unsigned int>(1);
                                if(presenceCount != 0){
                                    if(n == 0){ //node has re-joined bus
                                        clear();
                                        events = 1 << 7; //record that it has been cleared
                                    } else {
                                        if(presenceCount == UINT_MAX){
                                            expectedValue = n == 1;
                                        } else {
                                            expectedValue = n == (presenceCount + 1);
                                        }
                                        setStatusBit(2, !expectedValue);
                                    }
                                } 
                                presenceCount = n;
                                break;

                            case ArduinoMessage::TYPE_STATUS_RESPONSE:
                                statusResponseCount++;
                                setStatusBit(3, statusResponseCount != statusRequestCount);
                                break;

                            case ArduinoMessage::TYPE_PING_RESPONSE:
                                pingResponseCount++;
                                setStatusBit(5, pingResponseCount != pingSentCount);
                                break;
                        }
                        messageReceivedCount++;
                    }

                    void sent(ArduinoMessage* message){
                        switch(message->type){
                            case ArduinoMessage::TYPE_STATUS_REQUEST:
                                setStatusBit(4, statusResponseCount != statusRequestCount);
                                statusRequestCount++;
                                break;

                             case ArduinoMessage::TYPE_PING:
                                setStatusBit(5, pingResponseCount != pingSentCount);
                                pingSentCount++;
                                break;
                        }
                        messageSentCount++;
                    }
            };
            
            typedef void (*ForwardingListener)(MCP2515Monitor*, ArduinoMessage*); 

        private:
            //ArduinoMessage frecvmsg;
            //ArduinoMessage fsendmsg;

            ArduinoMessage* outboundMessage = NULL;

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

            void setOutboundMessage(ArduinoMessage* message){ outboundMessage = message; }
            bool canForwardMessages(){ return (outboundMessage != NULL && canForward); }
            void addForwardingListener(ForwardingListener listener){ forwardingListener = listener; }
            NodeData* getNodeData(byte nodeID, bool createIfNotFound);
            
            void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response) override;
            void forwardMessage(ArduinoMessage* message, byte messageID);
            
            void setReportInfo(ArduinoMessage* message) override;
            void setStatusInfo(ArduinoMessage* response) override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void onMessageReceived(byte sourceNodeID, ArduinoMessage *message) override;
            void onMessageSent(ArduinoMessage *message) override;
    };
} //end namespace
#endif //end prevent multiple inclusion