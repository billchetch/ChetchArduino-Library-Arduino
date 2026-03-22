#include "ChetchCANBusMonitor.h"

namespace Chetch{
    
    CANBusMonitor::CANBusMonitor(byte nodeID, byte serialPin) : CANBusBase(&mcp, &spin),
                                            mcp(nodeID, NODE_PRESENCE_INTERVAL),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) 
    {

        //empty

    }

}