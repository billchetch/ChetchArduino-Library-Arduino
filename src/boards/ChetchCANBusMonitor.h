#ifndef CHETCH_CAN_BUS_MONITOR_H
#define CHETCH_CAN_BUS_MONITOR_H

#include <limits.h>
#include "ChetchCANBusBase.h"
#include "devices/comms/can/ChetchMCP2515Device.h"
#include "devices/comms/serial/ChetchSerialPinSlave.h"
#include "ChetchArduinoIO.h"

/*
A can bus monitor board uses the monitor mcp to wrap bus messages and route them to the stream
*/


namespace Chetch{

    class CANBusMonitor : public CANBusBase{
        public:
            static const byte MESSAGE_ID_FORWARD_RECEIVED = 100;
            static const byte MESSAGE_ID_FORWARD_SENT = 101;
            
            static const unsigned int FORWARD_TIMEOUT = 20000; //compared to last status request
            
            static const int DEFAULT_CS_PIN = 6; //To free up pins 8 and 9 for AltSoftSerial

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

        protected:
            MCP2515Device mcp;
            SerialPinSlave spin;
            ArduinoIO io;

        private:
            bool canForward = false;
            //ForwardingListener forwardingListener;

            unsigned long lastStatusRequest = 0;
            bool statusRequested = false;
            unsigned int messageCount = 0;

            NodeData* firstNodeData = NULL;
            byte nodeCount = 0;
            NodeData* node2report = NULL;

        public:
            CANBusMonitor(byte nodeID, byte serialPin);

            bool begin(Stream *stream, byte framePadding = 0);
            void loop() override;

            void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response) override;
            bool executeCommand(ArduinoDevice::DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;
            void setReportInfo(ArduinoMessage* message) override;
            void forwardCANBusMessage(ArduinoMessage* message, byte messageID, unsigned long canID, byte* canData, byte canDLC);

            NodeData* getNodeData(byte nodeID, bool createIfNotFound);
    };
} //end namespace
#endif
