#include "ChetchCANBusMonitor.h"

namespace Chetch{
    
    CANBusMonitor::CANBusMonitor(byte serialPin) : CANBusBase(&mcp, &spin),
                                            mcp(NODE_PRESENCE_INTERVAL),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) {

        //addDevice(&mcp);
        //addDevice(&spin);

    }

}