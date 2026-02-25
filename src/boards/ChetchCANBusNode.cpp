#include "ChetchCANBUSNode.h"

namespace Chetch{
    //unsigned int CANBusNode::PresenceInterval = 10000;

    CANBusNode::CANBusNode(byte nodeID, byte serialPin) : CANBusBase(&mcp,  &spin),
                                            mcp(nodeID, NODE_PRESENCE_INTERVAL),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) {

        //empty
    }

}