#include "ChetchCANBUSNode.h"

namespace Chetch{
    //unsigned int CANBusNode::PresenceInterval = 10000;

    CANBusNode::CANBusNode(byte nodeID, byte serialPin) : CANBusBase(&mcp,  &spin),
                                            mcp(nodeID, MCP2515Node::DEFAULT_CS_PIN, NODE_PRESENCE_INTERVAL),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) 
    {
        addDevice(&mcp);
        addDevice(&spin);   

        mcp.setReportInterval(1000);
    }

    bool CANBusNode::begin(MessageIO* io){
        if(io == NULL){
            io = new CANBusIO(getMCP());
        }
        return CANBusBase::begin(io);
    }
}