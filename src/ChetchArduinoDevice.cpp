#include "ChetchArduinoDevice.h"
#include "ChetchArduinoBoard.h"
#include "ChetchArduinoMessage.h"

namespace Chetch{
    class ArduinoBoard;

    void ArduinoDevice::addEventListener(EventListener listener) {
        eventListener = listener;
    }

    bool ArduinoDevice::raiseEvent(byte eventID) {
        if (eventListener != NULL) {
            return eventListener(this, eventID);
        }
        return false;
    }

    void ArduinoDevice::setErrorInfo(ArduinoMessage* message, byte errorSubCode){
#if ARDUINO_BOARD_USE_STREAM
        Board->setErrorInfo(message, ArduinoBoard::ErrorCode::DEVICE_ERROR, errorSubCode);
#endif
    }

    void ArduinoDevice::setStatusInfo(ArduinoMessage* message){
        message->add(reportInterval);
    }

    void ArduinoDevice::setPingInfo(ArduinoMessage* message){
        message->add(millis());
    }

    void ArduinoDevice::handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response){
        switch(message->type){
            case ArduinoMessage::TYPE_INITIALISE:
                response->type = ArduinoMessage::TYPE_INITIALISE_RESPONSE;
                break;

            case ArduinoMessage::TYPE_STATUS_REQUEST:
                response->type = ArduinoMessage::TYPE_STATUS_RESPONSE;
                setStatusInfo(response);
                break;

            case ArduinoMessage::TYPE_PING:
                response->type = ArduinoMessage::TYPE_PING_RESPONSE;
                setPingInfo(response);
                break;

            case ArduinoMessage::TYPE_COMMAND:
                DeviceCommand command = (DeviceCommand)message->get<DeviceCommand>(0);
                if(executeCommand(command, message, response)){
                    response->type = ArduinoMessage::TYPE_COMMAND_RESPONSE;
                    response->add((byte)command);
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

    bool ArduinoDevice::enqueueMessageToSend(byte messageID, byte messageTag){
#if ARDUINO_BOARD_USE_STREAM
        return Board->enqueueMessageToSend(this, messageID, messageTag);
#endif
    }

    bool ArduinoDevice::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = false;
        switch(command){
            case SET_REPORT_INTERVAL:
                setReportInterval(message->get<int>(1));
                response->add(reportInterval);
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