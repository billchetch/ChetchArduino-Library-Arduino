#include "ChetchArduinoDevice.h"
#include "ChetchArduinoBoard.h"
#include "ChetchArduinoMessage.h"

namespace Chetch{
    class ArduinoBoard;

    void ArduinoDevice::addEventListener(EventListener listener) {
        eventListener = listener;
    }

    bool ArduinoDevice::raiseEvent(int eventID) {
        if (eventListener != NULL) {
            return eventListener(this, eventID);
        }
        return false;
    }

    void ArduinoDevice::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        switch(message->type){
            case ArduinoMessage::TYPE_STATUS_REQUEST:
                break;

            case ArduinoMessage::TYPE_COMMAND:
                DeviceCommand command = (DeviceCommand)message->get<DeviceCommand>(0);
                if(executeCommand(command, message, response)){
                    response->type = ArduinoMessage::TYPE_COMMAND_RESPONSE;
                    response->add((byte)command);

                } else {
                    response->clear();
                }
                break;
        }
    }

    void ArduinoDevice::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        if(messageID == MESSAGE_ID_REPORT){
            message->type = ArduinoMessage::TYPE_DATA;
            //Serial.println("Populating outbound message");
            setReportInfo(message);
        }        
    }

    bool ArduinoDevice::enqueueMessageToSend(byte messageID){
        return Board->enqueueMessageToSend(this, messageID);
    }

    bool ArduinoDevice::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = false;
        switch(command){
            case SET_REPORT_INTERVAL:
                //setReportInterval(message->argumentAsInt(argIdx));
                //response->addInt(reportInterval);
                handled = true;
                break;

            default:
                handled = false;
                break;
        }
                
        return handled;
    }


    void ArduinoDevice::loop(){
        if(reportInterval > 0 && millis() - lastMillis >= reportInterval){
            //Serial.println("Ok message is ready to send");
            lastMillis = millis();
            enqueueMessageToSend(MESSAGE_ID_REPORT);
        }
    }
} //end namespace scope