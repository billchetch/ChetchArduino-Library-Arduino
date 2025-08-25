#include "ChetchUtils.h"
#include "ChetchMCP2515Master.h"


namespace Chetch{
    MCP2515Master::MCP2515Master(int csPin) : MCP2515Device(MASTER_NODE_ID, csPin)
                                            , frecvmsg(ARDUINO_MESSAGE_SIZE + 12) //Add 12 bytes to allow for additional 'meta' data
                                            , fsendmsg(ARDUINO_MESSAGE_SIZE + 12) //Add 12 bytes to allow for additional 'meta' data

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

    void MCP2515Master::raiseError(MCP2515ErrorCode errorCode){
        MCP2515Device::raiseError(errorCode);

        enqueueMessageToSend(MESSAGE_ID_REPORT_ERROR, MESSAGE_ID_REPORT_ERROR);
    }

    void MCP2515Master::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        message->add(getNodeID());
        message->add((byte)mcp2515.getStatus());
        message->add((byte)mcp2515.getErrorFlags());
        message->add((byte)mcp2515.errorCountTX());
        message->add((byte)mcp2515.errorCountRX());
        message->add(canSend);
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
        } else if(messageID == MESSAGE_ID_REPORT_ERROR){
            setErrorInfo(message, lastError);
            message->add((byte)mcp2515.getErrorFlags());
            message->add((byte)mcp2515.errorCountTX());
            message->add((byte)mcp2515.errorCountRX());
        }
    }

    bool MCP2515Master::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = ArduinoDevice::executeCommand(command, message, response);
        
        if(!handled)
        {
            ArduinoMessage* msg;
            switch(command){
                case ArduinoDevice::REQUEST: //Message from outside BUS .. all nodes should respond to this
                    msg = getMessageForDevice(this, ArduinoMessage::TYPE_STATUS_REQUEST, 1);
                    sendMessage(msg, MCP2515Device::CAN_PRIORITY_LOW);
                    handled = true;
                    break;

                case ArduinoDevice::SYNCHRONISE: //Message from outside BUS .. all nodes should respond to this
                    msg = getMessageForDevice(this, ArduinoMessage::TYPE_COMMAND, 1);
                    msg->add((byte)command);
                    sendMessage(msg, MCP2515Device::CAN_PRIORITY_HIGH);

                    MCP2515Device::handleReceivedMessage(MCP2515Device::CAN_PRIORITY_HIGH, getNodeID(), msg);
                    
                    handled = true;
                    break;
            }
        }
        return handled;
    }

    bool MCP2515Master::sendMessage(ArduinoMessage *message, CANMessagePriority messagePriority){
        if(MCP2515Device::sendMessage(message, messagePriority)){
            fsendmsg.clear();
            fsendmsg.copy(message);
            fsendmsg.add(canOutFrame.can_id);
            fsendmsg.add((byte)canOutFrame.can_dlc);
            fsendmsg.add(message->type);
            enqueueMessageToSend(MESSAGE_ID_FORWARD_SENT, MESSAGE_ID_FORWARD_SENT);
            return true;
        } else {
            return false;
        }
    }

    void MCP2515Master::handleReceivedMessage(CANMessagePriority messagePriority, byte sourceNodeID, ArduinoMessage *message){
        //Make sure we take a copy of this message if we are forwarding stuff
        frecvmsg.clear();
        frecvmsg.copy(message);
        frecvmsg.add(canInFrame.can_id);
        frecvmsg.add((byte)canInFrame.can_dlc);
        frecvmsg.add(message->type);

        enqueueMessageToSend(MESSAGE_ID_FORWARD_RECEIVED, MESSAGE_ID_FORWARD_RECEIVED);

        MCP2515Device::handleReceivedMessage(messagePriority, sourceNodeID, message);
    }
}
