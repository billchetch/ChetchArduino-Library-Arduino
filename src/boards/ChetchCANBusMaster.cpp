#include "ChetchCANBusMaster.h"

namespace Chetch{
    
    CANBusMaster::CANBusMaster(byte serialPin) : CANBusBase(&mcp, &spin),
                                            mcp(MASTER_NODE_ID, NODE_PRESENCE_INTERVAL),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) 
    {

        //empty

    }

    bool CANBusMaster::begin(Stream* stream){
        io.begin(stream);
        return CANBusBase::begin(&io);
    }
}