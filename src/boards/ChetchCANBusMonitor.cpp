#include "ChetchCANBusMonitor.h"

namespace Chetch{
    
    CANBusMonitor::CANBusMonitor(byte nodeID, byte serialPin) : CANBusBase(&mcp, &spin),
                                            mcp(nodeID, NODE_PRESENCE_INTERVAL),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) 
    {

        addDevice(&mcp);
        addDevice(&spin);   
    }

    bool CANBusMonitor::begin(Stream* stream){
        io.begin(stream);
        return CANBusBase::begin(&io);
    }
}