#include "boards/ChetchCANBusIO.h"

namespace Chetch{
    CANBusIO::CANBusIO(MCP2515Device* mcp){ 
        this->mcp = mcp; 

        mcp->addMessageReceivedListener([](MCP2515Device* mcpDev, byte sourceNodeID, ArduinoMessage* msg, can_frame* canFrame){
            CANBusIO* io = (CANBusIO*)mcpDev->Board->getIO();
            io->handleReceivedBusMessage(sourceNodeID, msg, canFrame);
        });
    }

    void CANBusIO::loop(){
        if(!isMessageQueueEmpty()){
            ArduinoIO::MessageQueueItem* qi = &messageQueue[queueStart];
            //Serial.println("Sending a message from IO");

            ArduinoMessage* msg = mcp->getMessageForHandler(qi->handler->getID(), ArduinoMessage::TYPE_NONE, qi->messageTag);
            qi->handler->populateOutboundMessage(msg, qi->messageID);
            MCP2515Device::MCP2515ErrorCode err = mcp->sendMessage(msg, false);
            if(err == MCP2515Device::MCP2515ErrorCode::NO_ERROR){
                qi->handler->onMessageSent(msg, qi->messageID);

                //decremeent queue
                queueCount--;
                if(!isMessageQueueEmpty()){
                    queueStart = (queueStart + 1) % CB_QUEUE_SIZE;
                }
            }
        }

        //now we read a message
        mcp->readMessage();
    }

    void CANBusIO::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, can_frame* canFrame){
        Serial.print("Received bus message from");
        Serial.print(sourceNodeID);
        Serial.println(message->type);

        /*ArduinoBoard* Board = (ArduinoBoard*)owner;
        ArduinoMessage* response = NULL:
        int targetNode = -1;
        byte nodeID = mcp->getNodeID();


        if(message->sender == Board->getID()){
                    //NOTE: we respond to the board based request here as using the defulat board
                    //setStatusInfo message populator may not work as its desigened to reply to direct messaging
                    Board->handleInboundMessage(message, response)
                } else {
                    device = Board->getDeviceByID(message->sender);
                    if(device == NULL){
                        raiseError(UNKNOWN_RECEIVE_ERROR, 1);
                    } else {
                        response = getMessageForDevice(device, ArduinoMessage::TYPE_STATUS_RESPONSE);
                        device->setStatusInfo(response);
                        if(device->getID() == getID())this->statusRequestCount++;
                        if(sendMessage(response) == MCP2515ErrorCode::NO_ERROR){
                            if(device->getID() == getID())this->statusResponseCount++;
                        }
                        handled = true;
                    }
                }

        switch(message->type){
            case TYPE_STATUS_REQUEST:
            case TYPE_INITIALISE:
            case TYPE_PING:
                message->populate<byte>(canFrame->data);
                targetNode = (int)message->getLast<byte>();
                break;

            default:
                break;
        }    
            
            if(targetNode == 0 || targetNode == nodeID){
                if(message->sender == Board->getID()){
                    response = mcp->getMessageForBoard(ArduinoMessage::TYPE_NONE);
                    Board->handleInboundMessage(message, response);
                } else {
                    device = Board->getDeviceByID(message->sender);
                    if(device == NULL){
                        raiseError(UNKNOWN_RECEIVE_ERROR, 1);
                    } else {
                        response = getMessageForDevice(device, ArduinoMessage::TYPE_STATUS_RESPONSE);
                        device->setStatusInfo(response);
                        if(device->getID() == getID())this->statusRequestCount++;
                        if(sendMessage(response) == MCP2515ErrorCode::NO_ERROR){
                            if(device->getID() == getID())this->statusResponseCount++;
                        }
                        handled = true;
                    }
                }
            }
        } else if(message->type == ArduinoMessage::TYPE_INITIALISE){
            message->populate<byte>(canInFrame.data); //node id
            targetNode = message->getLast<byte>();
            if(targetNode == 0 || targetNode == getNodeID()){
                indicate(true);    
                remoteInitialised = true;
            }
        } else if(message->type == ArduinoMessage::TYPE_RESET){
            if(canInFrame.can_dlc != 2){
                raiseError(INVALID_MESSAGE, 12);
                return;
            }
            message->populate<byte, byte>(canInFrame.data); //reset regime + node id
            targetNode = message->getLast<byte>();
            if(targetNode == 0 || targetNode == getNodeID()){
                indicate(true);
                ResetRegime regime = message->get<ResetRegime>(0);
                switch(regime){
                    case RESET_ERRORS:
                        resetErrors();
                        break;

                    case RESET_DEVICE:
                        break;

                    default:
                        break;
                }
                remoteReset = true;
            }
        } else if(message->type ==  ArduinoMessage::TYPE_PING){
            message->populate<byte>(canInFrame.data);
            targetNode = message->getLast<byte>();
            if(targetNode == 0 || targetNode == getNodeID()){
                indicate(true);    
                pinged = true;
                handled = true;
            } 
        } else if(message->type == ArduinoMessage::TYPE_ERROR_TEST && canInFrame.can_dlc > 1){
            unsigned long edata = 0;
            if(canInFrame.can_dlc == 2){ //error code + node
                message->populate<byte, byte>(canInFrame.data);
            } else if(canInFrame.can_dlc == 6){ //erro code + error data + node
                message->populate<byte, unsigned long, byte>(canInFrame.data);
                edata = message->get<unsigned long>(1);
            } else {
                raiseError(INVALID_MESSAGE, 2);
                return;
            }
            targetNode = message->getLast<byte>();
            if(targetNode == 0 || targetNode == getNodeID()){
                indicate(true);
                MCP2515ErrorCode ecode = message->get<MCP2515ErrorCode>(0);
                raiseError(ecode, edata);
            }
        } else if(imsg.type == ArduinoMessage::MessageType::TYPE_PRESENCE){
            if(imsg.getArgumentCount() == 0){
                imsg.populate<unsigned long, unsigned int, unsigned int>(canInFrame.data);
            }
        } else if(message->type ==  ArduinoMessage::TYPE_COMMAND && canInFrame.can_dlc > 1){
            if(canInFrame.can_dlc == 2){ //command + node
                message->populate<byte, byte>(canInFrame.data);
            } else if(canInFrame.can_dlc == 3){ //command + byte arg + node
                message->populate<byte, byte, byte>(canInFrame.data);
            } else if(canInFrame.can_dlc == 4){ //command + int arg + node
                message->populate<byte, int, byte>(canInFrame.data);
            } else {
                raiseError(INVALID_MESSAGE, 3);
                return;
            }
            targetNode = message->getLast<byte>();
            command = message->get<ArduinoDevice::DeviceCommand>(0);
            if(targetNode == getNodeID()){
                device = Board->getDeviceByID(message->sender);
                if(device == NULL){
                    raiseError(UNKNOWN_RECEIVE_ERROR, 3);
                } else {
                    response = getMessageForHandler(message->sender, ArduinoMessage::TYPE_COMMAND_RESPONSE);
                    response->add(command);
                    if(device->executeCommand(command, message, response)){
                        sendMessage(response);
                    }
                    handled = true;
                }
            }
        } */
    }

    bool CANBusIO::isMessageQueueFull(){
        return queueCount >= CB_QUEUE_SIZE;
    }

    bool CANBusIO::isMessageQueueEmpty(){
        return queueCount == 0;
    }

    bool CANBusIO::enqueueMessageToSend(void* handler, byte messageID, byte messageTag){
        if(isMessageQueueFull()){
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