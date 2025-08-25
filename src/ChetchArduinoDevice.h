#ifndef CHETCH_ARDUINO_DEVICE_H
#define CHETCH_ARDUINO_DEVICE_H

#include <Arduino.h>
#include "ChetchArduinoMessage.h"


namespace Chetch{
    class ArduinoBoard;

    class ArduinoDevice{
        public:
            enum DeviceCommand{
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
                SYNCHRONISE
            };

        public:
            static const byte MESSAGE_ID_REPORT = 1;

            typedef bool (*EventListener)(ArduinoDevice*, byte);

        public:
            ArduinoBoard* Board = NULL; //TODO: this could be static to save space
            byte id = 0;

        private:
            EventListener eventListener = NULL;

        protected:
            bool begun = false;
            unsigned long lastMillis = 0;
            int reportInterval = -1; //measured in millis. negative or zero means no reporting
        
        public:
            void addEventListener(EventListener listener);
            bool raiseEvent(byte eventID);

            void setReportInterval(int interval) { reportInterval = interval; }
            
            bool enqueueMessageToSend(byte messageID, byte messageTag = 0);
            
            virtual void handleInboundMessage(ArduinoMessage* message, ArduinoMessage* response);
            virtual void populateOutboundMessage(ArduinoMessage* message, byte messageID);
            
            void setErrorInfo(ArduinoMessage* message, byte errorSubCode);
            virtual void setReportInfo(ArduinoMessage* message){}; //a hook
            virtual void setStatusInfo(ArduinoMessage* message);

            virtual bool executeCommand(DeviceCommand command, ArduinoMessage* message, ArduinoMessage* response);

            virtual bool begin() = 0;
            bool hasBegun(){ return begun; }
            virtual void loop();
    };
} //end namespace scope
#endif