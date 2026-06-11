#include "ChetchCANBusMaster.h"

namespace Chetch{
    
    CANBusMaster::CANBusMaster(byte serialPin) : CANBusBase(&mcp, &spin),
                                            mcp(MASTER_NODE_ID, DEFAULT_CS_PIN, 0),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) 
    {

        addDevice(&mcp);
        addDevice(&spin);
    }

    bool CANBusMaster::begin(Stream* stream){
        io.begin(stream);
        return CANBusBase::begin(&io);
    }
}