#include "ChetchCANBusMonitor.h"

namespace Chetch{
    
    CANBusMonitor::CANBusMonitor(byte nodeID, byte serialPin) : CANBusBase(&mcp, &spin),
                                            mcp(nodeID, DEFAULT_CS_PIN, 0),
                                            spin(serialPin, SERIAL_PIN_INTERVAL, SERIAL_PIN_BUFFER_SIZE) 
    {

        addDevice(&mcp);
        addDevice(&spin);

        setReportInterval(1000);
    }

    bool CANBusMonitor::begin(Stream* stream, byte framePadding){
        io.begin(stream, framePadding);

        mcp.addSendValidator([](MCP2515Device* dev, ArduinoMessage* msg, unsigned long canID, byte* canData, byte canDLC){
            CANBusMonitor* bm = (CANBusMonitor*)dev->Board;
            if(msg->getArgumentCount() > 0){
                byte targetNodeID = msg->getLast<byte>();
                CANBusMonitor::NodeData* nd = bm->getNodeData(targetNodeID, true);
                if(nd != NULL)nd->sent(msg);
            }
            bm->forwardCANBusMessage(msg, MESSAGE_ID_FORWARD_SENT, canID, canData, canDLC);
            return true;
        });
        mcp.addMessageReceivedListener([](MCP2515Device* dev, byte sourceNodeID, ArduinoMessage* msg, unsigned long canID, byte* canData, byte canDLC){
            CANBusMonitor* bm = (CANBusMonitor*)dev->Board;
            
            //record node data
            CANBusMonitor::NodeData* nd = bm->getNodeData(sourceNodeID, true);
            if(nd != NULL){
                nd->received(msg);
            }
            
            bm->forwardCANBusMessage(msg, MESSAGE_ID_FORWARD_RECEIVED, canID, canData, canDLC);
        });

        return CANBusBase::begin(&io);
    }

    void CANBusMonitor::loop(){
        CANBusBase::loop();

        if(statusRequested){
            if(millis() - lastStatusRequest > FORWARD_TIMEOUT){
                if(canForward){
                    canForward = false;
                    //raiseEvent(EVENT_FORWARDING_SET, canForward);
                }
            } else {
                if(!canForward){
                    canForward = true;
                    //raiseEvent(EVENT_FORWARDING_SET, canForward);
                }
            }
        }
       
        if(begun){
            getMCP()->readMessage();
        }
    }

    void CANBusMonitor::setReportInfo(ArduinoMessage* message){
        CANBusBase::setReportInfo(message);

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
            message->add(node2report->statusRequestCount);
            message->add(node2report->statusResponseCount);
            message->add(node2report->pingSentCount);
            message->add(node2report->pingResponseCount);
            message->add(node2report->messageReceivedCount);
            message->add(node2report->messageSentCount);
            
            node2report = node2report->nextNode;
        }

        messageCount = 0;
    }

    void CANBusMonitor::forwardCANBusMessage(ArduinoMessage* message, byte messageID, unsigned long canID, byte* canData, byte canDLC){
        if(!canForward){
            return;
        }

        //FLUSH: If main IO has a message pending then send it first
        ArduinoMessage* outboundMessage = io.getOutboundMessage();
        if(!outboundMessage->isEmpty()){
            io.sendMessage();
        }

        outboundMessage->type = ArduinoMessage::MessageType::TYPE_INFO;
        outboundMessage->tag = messageID;
        outboundMessage->sender = this->getID(); //because copying will have overwriten this data
        outboundMessage->target = this->getID();
        outboundMessage->addBytes(canData, canDLC);
        outboundMessage->add(canID);
        outboundMessage->add(message->type);
        
        //Note: we add the message sender value rather than rely on can_id as the value stored in can_id
        //is modified so as to only use 3 bits (see MCP2515Device for how IDs are packaged)
        if(messageID == MESSAGE_ID_FORWARD_SENT){
            if(message->sender == 0){
                outboundMessage->add(getID());    
            } else {
                outboundMessage->add((byte)(message->sender + ArduinoBoard::START_DEVICE_IDS_AT - 1));
            }
        } else {
            outboundMessage->add(message->sender);
        }

        /*if(messageID == MESSAGE_ID_FORWARD_RECEIVED){
            Serial.print("Forwarding received: ");
            Serial.print(message->type);
            Serial.print(" from ");
            Serial.println(outboundMessage->sender);
        }*/

        if(!io.sendMessage()){
            //Serial.println("ERRO enqueing message!");
            //raiseError(MCP2515ErrorCode::CUSTOM_ERROR, 10);
        }
    }

    void CANBusMonitor::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        CANBusBase::handleInboundMessage(message, response);

        switch(message->type){
            case ArduinoMessage::TYPE_STATUS_REQUEST:
                lastStatusRequest = millis();
                statusRequested = true;
                break;
        }
    }

    bool CANBusMonitor::executeCommand(ArduinoDevice::DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = CANBusBase::executeCommand(command, message, response);
        if(!handled && message->getArgumentCount() > 1)
        {
            byte nodeID = message->getLast<byte>();
            if(nodeID == getNodeID()){
                /*switch(command){
                    case ArduinoDevice::CLEAR:
                        {
                            byte node2clear = message->get<byte>(1);
                            NodeData* nd = getNodeData(node2clear, false);
                            if(nd != NULL){
                                nd->clear();
                                //TODO: enqueue report message to send to force quick update
                            }
                        }
                        break;

                    default:
                        break;
                }*/
            } else {
                ArduinoMessage* msg;
                ArduinoMessage::MessageType reqType;
                int byteTotal = 0;
                byte bytec = 0;
                byte startAt = 0;
                if(command == ArduinoDevice::REQUEST){
                    reqType = (ArduinoMessage::MessageType)message->get<ArduinoMessage::MessageType>(1);
                    msg = getMCP()->getMessageForHandler(message->sender, reqType, 1);
                    startAt = 2;
                    byteTotal = 0;
                } else {    
                    msg = getMCP()->getMessageForHandler(message->sender, ArduinoMessage::TYPE_COMMAND, 2);
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
                handled = getMCP()->sendMessage(msg) == MCP2515Device::NO_ERROR;
                canForward = oldCanForward;
                //This is done to ensure that the command response is directed back to the monitor device
                message->sender = this->getID();
            }
        }
        return handled;
    }

    CANBusMonitor::NodeData* CANBusMonitor::getNodeData(byte nodeID, boolean createIfNotFound){
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