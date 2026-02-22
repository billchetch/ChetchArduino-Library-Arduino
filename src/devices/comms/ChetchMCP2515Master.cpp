#include "ChetchUtils.h"
#include "ChetchMCP2515Master.h"

/*
 */

 
namespace Chetch{
    MCP2515Master::MCP2515Master(unsigned int presenceInterval, int csPin) : MCP2515Device(MASTER_NODE_ID, presenceInterval, csPin)
                                            , frecvmsg(22) //Add 12 bytes to allow for additional 'meta' data
                                            , fsendmsg(22) //Add 12 bytes to allow for additional 'meta' data

    { 
        setIndicateMode(NO_INDICATOR);
        setReportInterval(1000); //For reporting bus activity
    }

    bool MCP2515Master::begin(){
        if(getNodeID() != MASTER_NODE_ID){
            begun = false;
            return begun;
        } else {
            MCP2515Device::begin();
            return begun;
        }
	}

    void MCP2515Master::loop(){
        if(statusRequested){
            if(millis() - lastStatusRequest > FORWARD_TIMEOUT){
                if(canForward){
                    canForward = false;
                    raiseEvent(EVENT_FORWARDING_SET, canForward);
                }
            } else {
                if(!canForward){
                    canForward = true;
                    raiseEvent(EVENT_FORWARDING_SET, canForward);
                }
            }
        }

        MCP2515Device::loop();
    }
    void MCP2515Master::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        message->add(getNodeID());
        MCP2515Device::setStatusInfo(message);

        lastStatusRequest = millis();
        statusRequested = true;
    }

    void MCP2515Master::setReportInfo(ArduinoMessage* message){
        //MCP2515Device::setReportInfo(message);
        message->add(messageCount);
        messageCount = 0;
    }

    void MCP2515Master::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        ArduinoDevice::handleInboundMessage(message, response);

        if(message->type == ArduinoMessage::TYPE_PING){
            indicate(true);
        } else if(message->type == ArduinoMessage::TYPE_INITIALISE){
            indicate(true);
            resetErrors();
            if(messageReceivedListener != NULL){
                messageReceivedListener(this, getNodeID(), message, NULL);
            }
            response->add(millis());
            response->add((byte)TIMESTAMP_RESOLUTION);
            response->add(presenceInterval);
        } else if(message->type == ArduinoMessage::TYPE_FINALISE){
            indicate(true);
            canForward = false;
            statusRequested = false;
            if(messageReceivedListener != NULL){
                messageReceivedListener(this, getNodeID(), message, NULL);
            }
        } else if(message->type == ArduinoMessage::TYPE_ERROR_TEST){
            indicate(true);
            MCP2515ErrorCode ecode = message->get<MCP2515ErrorCode>(0);
            raiseError(ecode, 0); //message->get<unsigned long>(1));
            if(messageReceivedListener != NULL){
                messageReceivedListener(this, getNodeID(), message, NULL);
            }
        } else if(message->type== ArduinoMessage::TYPE_RESET){
            indicate(true);
            ResetRegime regime = message->get<ResetRegime>(0);
            
            init(true);

            resetErrors();
            if(clearReceive() > 2){
                raiseError(READ_FAIL, 3);
            }
            mcp2515.clearInterrupts();

            if(messageReceivedListener != NULL){
                messageReceivedListener(this, getNodeID(), message, NULL);
            }
        }
    }

    void MCP2515Master::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        MCP2515Device::populateOutboundMessage(message, messageID);

        if(messageID == MESSAGE_ID_FORWARD_RECEIVED){
            message->copy(&frecvmsg);
            //IMPORTANT: we identify forwarded messages as having the INFO type (original type is recorded as last parameter)
            message->type = ArduinoMessage::MessageType::TYPE_INFO;
        } else if(messageID == MESSAGE_ID_FORWARD_SENT){
            message->copy(&fsendmsg);
            //IMPORTANT: we identify forwarded messages as having the INFO type (original type is recorded as last parameter)
            message->type = ArduinoMessage::MessageType::TYPE_INFO;
        }
    }

    bool MCP2515Master::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = MCP2515Device::executeCommand(command, message, response);
        
        if(!handled)
        {
            ArduinoMessage* msg;
            ArduinoMessage::MessageType reqType;
            int byteTotal = 0;
            byte bytec = 0;
            byte startAt = 0;
            if(command == ArduinoDevice::REQUEST){
                reqType = (ArduinoMessage::MessageType)message->get<ArduinoMessage::MessageType>(1);
                msg = getMessageForDevice(message->sender, reqType, message->tag);
                startAt = 2;
                byteTotal = 0;
            } else {    
                msg = getMessageForDevice(message->sender, ArduinoMessage::TYPE_COMMAND, 1);
                msg->add((byte)command);
                startAt = 1;
                byteTotal = 1;
            }

            //copy message arguments across
            for(byte i = startAt; i < message->getArgumentCount(); i++){
                bytec = message->getArgumentSize(i);
                byteTotal += bytec;
                if(byteTotal >= CAN_MAX_DLC)break;
                msg->addBytes(message->getArgument(i), bytec);
            }

            //send using base method so as not to send a message back to the sender of this command
            handled = MCP2515Device::sendMessage(msg);
        }
        return handled;
    }
    
    bool MCP2515Master::sendMessage(ArduinoMessage *message){
        /*
        NOTE

        1. This method is used in MCP2515Device::handleReceivedMessage as well as MCP2525Device::loop

        2. Base sendMessage MUST be called in order to forward the message otherwise the message and canOutFrame are not seeded with data
        */
    
        if(MCP2515Device::sendMessage(message)){ 
            if(canForward){
                indicate(true);

                fsendmsg.clear();
                fsendmsg.tag = message->tag;
                
                fsendmsg.addBytes(canOutFrame.data, canOutFrame.can_dlc);
                
                fsendmsg.add(canOutFrame.can_id);
                fsendmsg.add(message->type);
                if(message->sender == 0){
                    fsendmsg.add(Board->getID());    
                } else {
                    fsendmsg.add((byte)(message->sender + ArduinoBoard::START_DEVICE_IDS_AT - 1));
                }
                
                messageCount++;

                if(!enqueueMessageToSend(MESSAGE_ID_FORWARD_SENT, MESSAGE_ID_FORWARD_SENT)){
                    raiseError(MCP2515ErrorCode::CUSTOM_ERROR, 10);
                }
            }
            return true;
        } else {
            return false;
        }
    }

    void MCP2515Master::handleReceivedMessage(byte sourceNodeID, ArduinoMessage *message){
        /*
        NOTE

        1. This will only be called by messages received from the CAN bus
        i.e only from remote nodes (Not from the serial connection).
        
        2. 0y here the message will have passed all error checks in MCP2515Device::readMessage
        So we can assume the message is valid
        */

        //Call base method first to allow default handling
        MCP2515Device::handleReceivedMessage(sourceNodeID, message);
        
        //update message count
        messageCount++;

        if(canForward){
            indicate(true);

            //Now we prepare a message to forward it vias the seria. connection
            frecvmsg.clear();
            frecvmsg.tag = message->tag;
            
            frecvmsg.addBytes(canInFrame.data, canInFrame.can_dlc);
            
            frecvmsg.add(canInFrame.can_id);
            frecvmsg.add(message->type);
            frecvmsg.add(message->sender);
            
            if(!enqueueMessageToSend(MESSAGE_ID_FORWARD_RECEIVED, MESSAGE_ID_FORWARD_RECEIVED)){
                raiseError(MCP2515ErrorCode::CUSTOM_ERROR, 11);
            }
        }
    }
}