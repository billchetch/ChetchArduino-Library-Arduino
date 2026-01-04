#include "ChetchUtils.h"
#include "ChetchMCP2515Master.h"


namespace Chetch{
    MCP2515Master::MCP2515Master(unsigned int presenceInterval, int csPin) : MCP2515Device(MASTER_NODE_ID, presenceInterval, csPin)
                                            , frecvmsg(20) //Add 12 bytes to allow for additional 'meta' data
                                            , fsendmsg(20) //Add 12 bytes to allow for additional 'meta' data

    { }

    bool MCP2515Master::begin(){
        if(getNodeID() != MASTER_NODE_ID){
            begun = false;
            return begun;
        } else {
            return MCP2515Device::begin();
        }
	}

    bool MCP2515Master::allowSending(){
        if(MCP2515Device::allowSending()){ //true if called for first time
            enqueueMessageToSend(MESSAGE_ID_READY_TO_SEND, MESSAGE_ID_READY_TO_SEND);
            return true;
        } else {
            return false;
        }
    }
 
    void MCP2515Master::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        message->add(getNodeID());
        MCP2515Device::setStatusInfo(message);
    }

    void MCP2515Master::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        ArduinoDevice::handleInboundMessage(message, response);
        switch(message->type){
            case ArduinoMessage::TYPE_PING:
                indicate(true);
                break;

            case ArduinoMessage::TYPE_INITIALISE:
                indicate(true);
                resetErrors();
                if(messageReceivedListener != NULL){
                    messageReceivedListener(this, getNodeID(), message, NULL);
                }
                response->add(millis());
                response->add((byte)TIMESTAMP_RESOLUTION);
                break;

            case ArduinoMessage::TYPE_RESET:
                indicate(true);
                resetErrors();
                if(clearReceive() > 2){
                    raiseError(READ_FAIL, 3);
                }
                mcp2515.clearInterrupts();
                if(messageReceivedListener != NULL){
                    messageReceivedListener(this, getNodeID(), message, NULL);
                }
                break;

            case ArduinoMessage::TYPE_ERROR_TEST:
                indicate(true);
                MCP2515ErrorCode ecode = message->get<MCP2515ErrorCode>(0);
                raiseError(ecode, message->get<unsigned long>(1));
                break;
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
        } else if(messageID == MESSAGE_ID_READY_TO_SEND){
            message->type = ArduinoMessage::MessageType::TYPE_NOTIFICATION;
        }
    }

    bool MCP2515Master::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = ArduinoDevice::executeCommand(command, message, response);
        
        if(!handled)
        {
            ArduinoMessage* msg;
            ArduinoMessage::MessageType reqType;
            int byteTotal = 0;
            byte bytec = 0;
            switch(command){
                case ArduinoDevice::REQUEST: //Message from outside BUS .. all nodes should respond to this
                    reqType = (ArduinoMessage::MessageType)message->get<ArduinoMessage::MessageType>(1);
                    msg = getMessageForDevice(this, reqType, message->tag);
                    byteTotal = 0;
                    for(byte i = 1; i < message->getArgumentCount(); i++){
                        bytec = message->getArgumentSize(i + 1);
                        byteTotal += bytec;
                        if(byteTotal >= CAN_MAX_DLC)break;
                        msg->addBytes(message->getArgument(i + 1), bytec);
                    }

                    //send using base method so as not to send a message back to the sender of this command
                    MCP2515Device::sendMessage(msg);
                    handled = true;
                    break;

                default: //Message from outside BUS .. all nodes should respond to this
                    msg = getMessageForDevice(this, ArduinoMessage::TYPE_COMMAND, 1);
                    msg->add((byte)command);
                    byteTotal = 1;
                    for(byte i = 0; i < message->getArgumentCount(); i++){
                        bytec = message->getArgumentSize(i + 1);
                        byteTotal += bytec;
                        if(byteTotal >= CAN_MAX_DLC)break;
                        msg->addBytes(message->getArgument(i + 1), bytec);
                    }

                    //send using base method so as not to send a message back to the sender of this command
                    MCP2515Device::sendMessage(msg);

                    if(commandListener != NULL){
                        commandListener(this, getNodeID(), command, msg);
                    }

                    handled = true;
                    break;
            }
        }
        return handled;
    }
    
    bool MCP2515Master::sendMessage(ArduinoMessage *message){
        if(MCP2515Device::sendMessage(message)){
            fsendmsg.clear();
            fsendmsg.tag = message->tag;
            fsendmsg.sender = message->sender;

            fsendmsg.addBytes(canOutFrame.data, canOutFrame.can_dlc);
            
            fsendmsg.add(canOutFrame.can_id);
            fsendmsg.add(message->type);

            enqueueMessageToSend(MESSAGE_ID_FORWARD_SENT, MESSAGE_ID_FORWARD_SENT);
            return true;
        } else {
            return false;
        }
    }

    void MCP2515Master::handleReceivedMessage(byte sourceNodeID, ArduinoMessage *message){
        //TODO: check first if we have set any forwarding message filters
        
        //Call base method
        MCP2515Device::handleReceivedMessage(sourceNodeID, message);

        //Make sure we take a copy of this message if we are forwarding stuff
        frecvmsg.clear();
        frecvmsg.tag = message->tag;
        frecvmsg.sender = message->sender;

        frecvmsg.addBytes(canInFrame.data, canInFrame.can_dlc);
        
        frecvmsg.add(canInFrame.can_id);
        frecvmsg.add(message->type);

        enqueueMessageToSend(MESSAGE_ID_FORWARD_RECEIVED, MESSAGE_ID_FORWARD_RECEIVED);
    }
}