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
        setReportInterval(1000); //For reporting bus activity
    }

    MCP2515Monitor::~MCP2515Monitor(){
        if(firstNodeData != NULL){
            NodeData* nd = firstNodeData;
            NodeData* nextNode = NULL;
            do{
                nextNode = nd->nextNode;
                delete nd;
            } while(nextNode != NULL);
            delete firstNodeData;
        }
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
        MCP2515Device::setReportInfo(message);
        message->add(messageCount);
        message->add(nodeCount);

        if(firstNodeData == NULL){
            message->add((byte)0);
        } else {
            if(node2report == NULL)node2report = firstNodeData;
            
            message->add(node2report->nodeID);
            message->add(node2report->status);
            message->add(node2report->events);
            message->add(node2report->presenceCount);
            message->add(node2report->statusResponseCount);
            message->add(node2report->statusRequestCount);
            message->add(node2report->messageReceivedCount);
            message->add(node2report->messageSentCount);
            
            node2report = node2report->nextNode;
        }

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
        if(messageID == MESSAGE_ID_FORWARD_RECEIVED || messageID == MESSAGE_ID_FORWARD_SENT){
            byte tag = message->tag;
            ArduinoMessage* fmsg = messageID == MESSAGE_ID_FORWARD_RECEIVED ? &frecvmsg : &fsendmsg;
            message->copy(fmsg);

            message->type = ArduinoMessage::MessageType::TYPE_INFO;
            message->tag = tag;
            message->sender = this->getID(); //because copying will have overwriten this data
            message->target = this->getID();
            

            if(forwardingListener != NULL){
                forwardingListener(this, message);
            }

            fmsg->clear();
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
            handled = sendMessage(msg);
            canForward = oldCanForward;
            //This is done to ensure that the command response is directed back to the monitor device
            message->sender = this->getID();
        }
        return handled;
    }

    void MCP2515Monitor::onMessageSent(ArduinoMessage *message){

        if(message->getArgumentCount() > 0){
            byte targetNodeID = message->getLast<byte>();
            NodeData* nd = getNodeData(targetNodeID, true);
            if(nd != NULL)nd->sent(message);
        }

        messageCount++;

        if(canForward){
            indicate(true);

            if(!fsendmsg.isEmpty()){
                raiseError(MCP2515ErrorCode::CUSTOM_ERROR, 14);
                //Serial.println("Send conflict!");
                return; 
            }

            fsendmsg.addBytes(canOutFrame.data, canOutFrame.can_dlc);
            fsendmsg.add(canOutFrame.can_id);
            fsendmsg.add(message->type);
            if(message->sender == 0){
                fsendmsg.add(Board->getID());    
            } else {
                fsendmsg.add((byte)(message->sender + ArduinoBoard::START_DEVICE_IDS_AT - 1));
            }
            
            if(!enqueueMessageToSend(MESSAGE_ID_FORWARD_SENT, MESSAGE_ID_FORWARD_SENT)){
                //Serial.println("Error Send failed to enqueue!");
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

        NodeData* nd = getNodeData(sourceNodeID, true);
        if(nd != NULL){
            nd->received(message);
        }
        
        //update message count
        messageCount++;

        if(canForward){
            indicate(true);

            //Now we prepare a message to forward it vias the seria. connection
            if(!frecvmsg.isEmpty()){
                raiseError(MCP2515ErrorCode::CUSTOM_ERROR, 15);
                return; 
            }
            

            frecvmsg.addBytes(canInFrame.data, canInFrame.can_dlc);
            frecvmsg.add(canInFrame.can_id);
            frecvmsg.add(message->type);
            frecvmsg.add(message->sender);

            
            if(!enqueueMessageToSend(MESSAGE_ID_FORWARD_RECEIVED, MESSAGE_ID_FORWARD_RECEIVED)){
                //Serial.println("ERRO enqueing message!");
                raiseError(MCP2515ErrorCode::CUSTOM_ERROR, 11);
            }
        }
    }

    MCP2515Monitor::NodeData* MCP2515Monitor::getNodeData(byte nodeID, boolean createIfNotFound){
        if(nodeID < MCP2515Device::MIN_NODE_ID || nodeID > MCP2515Device::MAX_NODE_ID){
            return NULL;
        }

        if(firstNodeData == NULL){
            if(createIfNotFound){
                firstNodeData = new NodeData(nodeID);
                nodeCount++;
            }
            return firstNodeData;
        }
        
        //Here we know firstNodeData is not NULL
        NodeData* nd = NULL;
        do{
            nd = nd == NULL ? firstNodeData : nd->nextNode;
            if(nd->nodeID == nodeID)return nd;
        }while(nd->nextNode != NULL);

        if(createIfNotFound){
            nd->nextNode = new NodeData(nodeID);
            nodeCount++;
            return nd->nextNode;
        } else {
            return NULL;
        }
    }
}