#include "ChetchCANBusMonitor.h"

namespace Chetch{
    
    CANBusMonitor::CANBusMonitor(byte nodeID, byte serialPin) : CANBusBase(&mcp, &spin),
                                            mcp(nodeID, MCP2515Monitor::DEFAULT_CS_PIN, NODE_PRESENCE_INTERVAL),
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