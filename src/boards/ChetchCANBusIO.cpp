#include "boards/ChetchCANBusIO.h"

namespace Chetch{
    CANBusIO::CANBusIO(MCP2515Device* mcp){ 
        this->mcp = mcp; 
    }

    void CANBusIO::setErrorBit(byte bitPosition, bool val){
        byte mask = 1 << bitPosition - 1;
        if(val){
            errorFlags = errorFlags | mask;
        } else {
            errorFlags = errorFlags & ~mask;
        }
    }

    void CANBusIO::loop(){
        ArduinoMessage* message;
        MCP2515Device::MCP2515ErrorCode err;

        if(!isMessageQueueEmpty()){
            ArduinoIO::MessageQueueItem* qi = &messageQueue[queueStart];
            //Serial.println("Sending a message from IO");

            message = mcp->getMessageForHandler(qi->handler->getID(), ArduinoMessage::TYPE_NONE, qi->messageTag);
            qi->handler->populateOutboundMessage(message, qi->messageID);
            //Serial.print("Deuqueing: ");
            //Serial.println(qi->messageID);
            err = mcp->sendMessage(message, false);
            if(err == MCP2515Device::MCP2515ErrorCode::NO_ERROR){
                qi->handler->onOutboundMessageSent(message, qi->messageID);

                //decremeent queue
                queueCount--;
                if(!isMessageQueueEmpty()){ //move queue position on one
                    queueStart = (queueStart + 1) % CB_QUEUE_SIZE;
                }
            } else {
                setErrorBit(2, true); //send fail
            }
        }

        //now we read a message and route it
        message = mcp->readMessage();
        if(message != NULL){ //we have received a valid and parsed message
            if(ArduinoMessage::getGroup(message) == ArduinoMessage::TARGETED_GROUP){
                byte targetNode = message->getLast<byte>();
                ArduinoBoard* board = (ArduinoBoard*)owner;
                ArduinoMessage* response = NULL;
                byte target = message->sender;

                if(targetNode == 0 || targetNode == mcp->getNodeID()){
                    if(target >= ArduinoBoard::START_DEVICE_IDS_AT){
                        ArduinoDevice* device = board->getDeviceByID(target);
                        if(device != NULL){
                            response = mcp->getMessageForDevice(device, ArduinoMessage::TYPE_NONE, message->tag);
                            device->handleInboundMessage(message, response);
                        } else {
                            //error
                        }
                    } else {
                        response = mcp->getMessageForBoard(ArduinoMessage::TYPE_NONE, message->tag);
                        board->handleInboundMessage(message, response);
                    }
                }

                //Now if the response is
                if(response != NULL && !response->isEmpty()){
                    /*Serial.print("Responding with ");
                    Serial.print(response->type);
                    Serial.print(" to ");
                    Serial.print(response->target);
                    Serial.print(" from ");
                    Serial.print(response->sender);
                    Serial.print(" args#: ");
                    Serial.println(response->getArgumentCount());*/
                    err = mcp->sendMessage(response, false);
                    if(err == MCP2515Device::MCP2515ErrorCode::NO_ERROR){
                        
                    } else {
                        //Serial.print("Error sending: ");
                        //Serial.println(err);
                        setErrorBit(3, true); //response fail
                    }
                }
            } else {
                //broadcast or misc messages
            }
        }
    } 

    bool CANBusIO::isMessageQueueFull(){
        return queueCount >= CB_QUEUE_SIZE;
    }

    bool CANBusIO::isMessageQueueEmpty(){
        return queueCount == 0;
    }

    bool CANBusIO::enqueueMessageToSend(void* handler, byte messageID, byte messageTag){
        if(isMessageQueueFull()){
            setErrorBit(1, true); //overflow
            return false;
        } else {
            int idx = (queueStart + queueCount) % CB_QUEUE_SIZE;
            messageQueue[idx].handler = (ArduinoMessageHandler*)handler;
            messageQueue[idx].messageID = messageID;
            messageQueue[idx].messageTag = messageTag;
            queueCount++;
            return true;
        }
    }
}