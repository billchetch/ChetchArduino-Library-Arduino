#include "ChetchUtils.h"
#include "ChetchMCP2515Monitor.h"

/*
 */

 
namespace Chetch{
    MCP2515Monitor::MCP2515Monitor(byte nodeID, int csPin, unsigned int presenceInterval) : MCP2515Device(nodeID, csPin, presenceInterval)
                                            , frecvmsg(24) 
                                            , fsendmsg(24) 

    { 
        setIndicateMode(NO_INDICATOR);
        //setReportInterval(1000); //For reporting bus activity
    }

    void MCP2515Monitor::loop(){
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

    void MCP2515Monitor::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        message->add(getNodeID());
        MCP2515Device::setStatusInfo(message);

        lastStatusRequest = millis();
        statusRequested = true;
    }

    void MCP2515Monitor::setReportInfo(ArduinoMessage* message){
        //MCP2515Device::setReportInfo(message);
        message->add(messageCount);
        messageCount = 0;
    }

    void MCP2515Monitor::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        ArduinoDevice::handleInboundMessage(message, response);

        if(message->type == ArduinoMessage::TYPE_PING){
            indicate(true);
        } else if(message->type == ArduinoMessage::TYPE_INITIALISE){
            indicate(true);
            resetErrors();
            
            response->add(millis());
            response->add((byte)TIMESTAMP_RESOLUTION);
            response->add(presenceInterval);
        } else if(message->type == ArduinoMessage::TYPE_FINALISE){
            indicate(true);
            canForward = false;
            statusRequested = false;
        } else if(message->type == ArduinoMessage::TYPE_ERROR_TEST){
            indicate(true);
            MCP2515ErrorCode ecode = message->get<MCP2515ErrorCode>(0);
            raiseError(ecode, 0); //message->get<unsigned long>(1));
        } else if(message->type== ArduinoMessage::TYPE_RESET){
            indicate(true);
            //ResetRegime regime = message->get<ResetRegime>(0);
            
            init(true);

            resetErrors();
            if(clearReceive() > 2){
                raiseError(READ_FAIL, 3);
            }
            mcp2515.clearInterrupts();
        }
    }

    void MCP2515Monitor::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        MCP2515Device::populateOutboundMessage(message, messageID);

        //IMPORTANT: we identify forwarded messages as having the INFO type
        if(messageID == MESSAGE_ID_FORWARD_RECEIVED){
            byte tag = message->tag;
            message->copy(&frecvmsg);
            message->type = ArduinoMessage::MessageType::TYPE_INFO;
            message->tag = tag;
            message->sender = this->getID(); //because copying will overwrite this data
            message->target = this->getID();
            
            if(forwardingListener != NULL){
                forwardingListener(this, message);
            }
        } else if(messageID == MESSAGE_ID_FORWARD_SENT){
            byte tag = message->tag;
            message->copy(&fsendmsg); //let the sender and target be determined by fsendmsg
            message->type = ArduinoMessage::MessageType::TYPE_INFO;
            message->tag = tag;
            message->sender = this->getID(); //because copying will overwrite this data
            message->target = this->getID();
            
            //IMPORTANT: we identify forwarded messages as having the INFO type (original type is recorded as last parameter)
            if(forwardingListener != NULL){
                forwardingListener(this, message);
            }
        }
    }

    bool MCP2515Monitor::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
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
                msg = getMessageForHandler(message->sender, reqType, 1);
                startAt = 2;
                byteTotal = 0;
            } else {    
                msg = getMessageForHandler(message->sender, ArduinoMessage::TYPE_COMMAND, 2);
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

            bool oldCanForward = canForward;
            canForward = false; //supress forwarding
            bool success = sendMessage(msg);
            response->add(success);
            canForward = oldCanForward;
            
            //This is done to ensure that the command response is directed back to the monitor device
            message->sender = this->getID();
            handled = true;
        }
        return handled;
    }
    
    void MCP2515Monitor::onMessageSent(ArduinoMessage *message){
        if(canForward){
            indicate(true);

            fsendmsg.clear();
            fsendmsg.addBytes(canOutFrame.data, canOutFrame.can_dlc);
            fsendmsg.add(canOutFrame.can_id);
            fsendmsg.add(message->type);
            if(message->sender == 0){
                fsendmsg.add(Board->getID());    
            } else {
                fsendmsg.add((byte)(message->sender + ArduinoBoard::START_DEVICE_IDS_AT - 1));
            }
            
            //Serial.print(">>CID:");
            //Serial.println(fsendmsg.get<unsigned long>(1));

            messageCount++;

            if(!enqueueMessageToSend(MESSAGE_ID_FORWARD_SENT, MESSAGE_ID_FORWARD_SENT)){
                raiseError(MCP2515ErrorCode::CUSTOM_ERROR, 10);
            }
        }
    }

    void MCP2515Monitor::onMessageReceived(byte sourceNodeID, ArduinoMessage *message){
        /*
        NOTE

        1. This will only be called by messages received from the CAN bus
        i.e only from remote nodes (Not from the serial connection).
        
        2. 0y here the message will have passed all error checks in MCP2515Device::readMessage
        So we can assume the message is valid
        */

        
        //update message count
        messageCount++;

        if(canForward){
            indicate(true);

            //Now we prepare a message to forward it vias the seria. connection
            frecvmsg.clear();
            
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