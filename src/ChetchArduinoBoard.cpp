#include "ChetchArduinoBoard.h"
#include <MemoryFree.h>

namespace Chetch{
    
    //Constructors
    ArduinoBoard::ArduinoBoard() 
    {
        setID(DEFAULT_BOARD_ID);

        for(byte i = 0; i < MAX_DEVICES; i++){
            devices[i] = NULL;
        }
    }

    bool ArduinoBoard::begin(MessageIO* io){
        if(io != NULL){
            io->begin(this);
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
            device->setID(i + START_DEVICE_IDS_AT);
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

    void ArduinoBoard::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        switch(message->type){
            case ArduinoMessage::TYPE_ECHO:
                response->type = ArduinoMessage::TYPE_ECHO_RESPONSE;
                response->copy(message);
                break;

            case ArduinoMessage::TYPE_PING:
                response->type = ArduinoMessage::TYPE_PING_RESPONSE;
                response->add(millis());
                break;

            case ArduinoMessage::TYPE_STATUS_REQUEST:
                response->type = ArduinoMessage::TYPE_STATUS_RESPONSE;
                setStatusInfo(response);
                break;

            case ArduinoMessage::TYPE_COMMAND:
                {
                    response->type = ArduinoMessage::TYPE_COMMAND_RESPONSE;
                    ArduinoDevice::DeviceCommand command = message->get<ArduinoDevice::DeviceCommand>(0);
                    response->add((byte)command);
                    bool success = executeCommand(command, message, response);
                    response->add(success); //success of not
                }
                break;

            default:
                //TODO: add error to response
                break;
        }
    }

    void ArduinoBoard::setStatusBit(byte bitPosition, bool val){
        byte mask = 1 << bitPosition - 1;
        if(val){
            statusFlags = statusFlags | mask;
        } else {
            statusFlags = statusFlags & ~mask;
        }
    }

    void ArduinoBoard::setStatusInfo(ArduinoMessage* message){
        if(message != NULL){
            message->add(millis());
            message->add(getDeviceCount());
            message->add(getFreeMemory());
            message->add(statusFlags);
        }
    }

    bool ArduinoBoard::executeCommand(ArduinoDevice::DeviceCommand command, ArduinoMessage* message, ArduinoMessage* response){
        return false;   
    }


    void ArduinoBoard::onReportReady(){
        if(io != NULL){
            io->enqueueMessageToSend(this, MESSAGE_ID_REPORT);
        }
    }

    void ArduinoBoard::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        if(messageID == MESSAGE_ID_REPORT){
            message->type = ArduinoMessage::TYPE_DATA;
            //Serial.println("Populating outbound message");
            setReportInfo(message);
        }        
    }

    void ArduinoBoard::loop(){
        if(!begun)return;

        //1. If we have an IO then loop for messages
        if(io != NULL){
            io->loop();
        }
      
        //2. Board stuff here
        if(reportInterval > 0 && millis() - lastMillis > reportInterval){
            lastMillis = millis();
            onReportReady();
        }

        //3. Loop next device
        if(deviceCount > 0 && devices[currentdevice] != NULL){
            devices[currentdevice]->loop(); //will update device state, possible set flags etc. to then pouplate outbound message
            currentdevice = (currentdevice + 1) % deviceCount;
        }
    }
}