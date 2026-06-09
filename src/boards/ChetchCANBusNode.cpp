#include "ChetchCANBUSNode.h"

namespace Chetch{
    //unsigned int CANBusNode::PresenceInterval = 10000;

    CANBusNode::CANBusNode(byte nodeID, byte serialPin) : CANBusBase(&mcp,  &spin),
                                            mcp(nodeID, DEFAULT_CS_PIN, NODE_PRESENCE_INTERVAL),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE),
                                            io(&mcp)
    {
        addDevice(&mcp);
        addDevice(&spin);   

        //mcp.setReportInterval(1000);
        mcp.setFilterPolicy(MCP2515Device::FilterPolicy::RESTRICT_TO_TARGETED);

        mcp.addMessageReceivedListener([](MCP2515Device* dev, byte sourceNodeID, ArduinoMessage* msg, unsigned long canID, byte* canData, byte canDLC){
            CANBusNode* bn = (CANBusNode*)dev->Board;
        
            bn->handleReceivedBusMessage(sourceNodeID, msg, canData);
        });
    }

    bool CANBusNode::begin(MessageIO* io){
        if(io != NULL){
            return false;
        }
        setStatusBit(8, true);
        return CANBusBase::begin(&this->io);
    }

    void CANBusNode::setStatusInfo(ArduinoMessage* message){
        byte sf = (0xF0 & getStatusFlags()) | (0x0F & io.getErrorFlags());
        setStatusFlags(sf);

        CANBusBase::setStatusInfo(message);
    }

    void CANBusNode::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData){
        /*Serial.print("Received from ");
        Serial.print(sourceNodeID);
        Serial.print(" type ");
        Serial.print(message->type);
        Serial.print(" from ");
        Serial.println(message->sender);*/
    }
}