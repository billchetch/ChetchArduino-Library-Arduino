#ifndef CHETCH_ARDUINO_DEVICE_H
#define CHETCH_ARDUINO_DEVICE_H

#include <Arduino.h>
#include "ChetchArduinoMessage.h"
#include "ChetchArduinoMessageHandler.h"


namespace Chetch{
    class ArduinoBoard;

    class ArduinoDevice : public ArduinoMessageHandler{
        public:
            enum DeviceCommand : byte{
                NONE = 0,
                COMPOUND,
                TEST,
                ENABLE,
                DISABLE,
                SET_REPORT_INTERVAL,
                START,
                STOP,
                PAUSE,
                RESET,
                ON,
                OFF,
                MOVE,
                ROTATE,
                PRINT,
                SET_CURSOR,
                DIZPLAY, //changed S to Z to avoid a define constant name clash
                CLEAR,
                SILENCE,
                SEND,
                TRANSMIT,
                SAVE,
                ACTIVATE,
                DEACTIVATE,
                RESUME,
                ZERO,
                ANALYSE,
                LOCK,
                UNLOCK,
                REQUEST,
                SYNCHRONISE,
                UPDATE,
                CALIBRATE,
                SET_MODE
            };

        public:
            static const byte MESSAGE_ID_REPORT = 1;
            static const byte EVENT_REPORT_READY = 1;

            typedef bool (*EventListener)(ArduinoDevice*, byte, byte); //device, event ID and event tag
        
        public:
            static ArduinoBoard* Board; 
            
        private:
            EventListener eventListener = NULL;
        
        protected:
            bool begun = false;
            unsigned long lastMillis = 0;
            int reportInterval = -1; //measured in millis. negative or zero means no reporting
        
        public:
            void addEventListener(EventListener listener);
            bool raiseEvent(byte eventID, byte eventTag = 0);
        
            void setReportInterval(int interval) { reportInterval = interval; }
            
            bool enqueueMessageToSend(byte messageID, byte messageTag = 0);
            
            void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response) override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            void onOutboundMessageSent(ArduinoMessage* message, byte messageID) override {};
            
            void setErrorInfo(ArduinoMessage* message, byte errorSubCode);
            virtual void setReportInfo(ArduinoMessage* message){}; //a hook so we don't have to override this method (not all devices report)
            virtual void setStatusInfo(ArduinoMessage* message);
            virtual void setPingInfo(ArduinoMessage* message);

            virtual bool executeCommand(DeviceCommand command, ArduinoMessage* message, ArduinoMessage* response);

            virtual bool begin() { begun = true; return begun; };
            bool hasBegun(){ return begun; }
            virtual void loop();
    };
} //end namespace scope
#endif