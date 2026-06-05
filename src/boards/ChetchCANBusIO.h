#ifndef CHETCH_CANBUS_IO_H
#define CHETCH_CANBUS_IO_H


#include <Arduino.h>
#include "ChetchMessageIO.h"
#include "ChetchArduinoIO.h"
#include "devices/comms/can/ChetchMCP2515Device.h"

namespace Chetch{
    class CANBusIO : public MessageIO{
        private:
            static const int CB_QUEUE_SIZE = 4;
        private:
            MCP2515Device* mcp = NULL;
            int queueStart = 0;
            int queueCount = 0;
            ArduinoIO::MessageQueueItem messageQueue[CB_QUEUE_SIZE];


        private:
            bool isMessageQueueFull();
            bool isMessageQueueEmpty();

        public:
            CANBusIO(MCP2515Device* mcp);

            bool enqueueMessageToSend(void* sender, byte messageID, byte messageTag = 0) override;
            bool sendMessage() override {}; //not used
            void loop() override;

            void handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, can_frame* canFrame);
    }; //end class
} //end namespace
#endif