#include "ChetchCANBusMonitor.h"

namespace Chetch{
    
    CANBusMonitor::CANBusMonitor(byte nodeID, byte serialPin) : CANBusBase(&mcp, &spin),
                                            mcp(nodeID),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) 
    {

        addDevice(&mcp);
        addDevice(&spin);
    }

    bool CANBusMonitor::begin(Stream* stream, byte framePadding){
        io.begin(stream, framePadding);

        mcp.setOutboundMessage(io.getOutboundMessage());
        return CANBusBase::begin(&io);
    }
}