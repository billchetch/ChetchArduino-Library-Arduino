#include "ChetchArduinoBoard.h"
#include <MemoryFree.h>

namespace Chetch{
    
    //Constructors
    ArduinoBoard::ArduinoBoard() 
    {
        for(byte i = 0; i < MAX_DEVICES; i++){
            devices[i] = NULL;
        }
    }

    bool ArduinoBoard::begin(ArduinoIOBase* io){
        if(io != NULL){
            io->Board = this;
            io->begin();
            this->io = io;
        }

        for(int i = 0; i < deviceCount; i++){
            if(!devices[i]->begin()){ //in case we forget to set the begun flag
                return false;
            }
        }
        
        begun = true;
        return begun;
    }

    void ArduinoBoard::addDevice(ArduinoDevice* device){
        byte i = deviceCount;
        if(i < MAX_DEVICES){
            devices[i] = device;
            device->id = i + START_DEVICE_IDS_AT;
            device->Board = this;
            deviceCount++;
        }
    }

    ArduinoDevice* ArduinoBoard::getDeviceByID(byte id){
        if(id >= START_DEVICE_IDS_AT && id < deviceCount + START_DEVICE_IDS_AT){
            return devices[id - START_DEVICE_IDS_AT];
        } else {
            return NULL;
        }
    }

    ArduinoDevice* ArduinoBoard::getDeviceAt(byte idx){
        if(idx >= 0 && idx < deviceCount){
            return devices[idx];
        } else {
            return NULL;
        }
    }

    int ArduinoBoard::getFreeMemory(){
        return freeMemory();
    }

    void ArduinoBoard::loop(){
        if(!begun)return;

        //1. If we have an IO then loop for messages
        if(io != NULL){
            io->loop();
        }
      
        //2. Loop next device
        if(deviceCount > 0 && devices[currentdevice] != NULL){
            devices[currentdevice]->loop(); //will update device state, possible set flags etc. to then pouplate outbound message
            currentdevice = (currentdevice + 1) % deviceCount;
        }
    }
}