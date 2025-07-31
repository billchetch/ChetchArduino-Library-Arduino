#include <Arduino.h>

#include <ChetchArduinoBoard.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>


#include <SPI.h>
#include <mcp2515.h>

/*
NANO
INT PIN = D2 (this is not required if looping)
CS PIN = D10
MOSI PIN = D11
MISO PIN = D12
SCK PIN = D13

This class is deisgned to:

1. Wrap an MCP2515 device in an ArduinoDevice class so it can be added to an ArduinoBoard
2. Perform message conversions between ArduinoMessage messages and CAN frames (Extended ID)

Wrapping MCP2515 device
Currently using the autowp library but this could change...

Message Conversion
The key to this is using the extended ID (so 29 bit) ID value as follows:

- Byte 1 = 3 bits for numbering the particular bus and 5 bits for the node.  This allows for 8 separate buses each with 32 max nodes.
- Byte 2 = 3 bits to indicate a type used to translate to the Arduino Message Type and 5 bits to prpresent a sender ID (hence 8 types 32 possible senders
- Byte 3 = Message structure:  2 bits for argument length, then 2 bits for arg 1 length, 2 bits for arg2 length and 2 bits for arg 3 length.

More on the message structure... in conjuntion with the can frame DLC value, Byte 3 allows for 4 possible arguments with each argument being of max 4 bytes. 
This corresponds to arduino basic types (float, int, long, byte etc.) It also allows for a varilable length argument if there is only 1 arg as we can use the DLC
value to determine

Free single argument example:
DLC = 0 to 8
Byte 3 = 00 00 00 00 00

2 arguments example (4 bytes and 2 bytes)
DLC = 6
Byte 3 = 01 11 01 00

3 arguments example (3 bytes and 4 bytes 1 byte)
DLC = 6
Byte 3 = 10 11 01 00

4 arguments example (2 bytes and 3 bytes 1 byte 2 byte)
DLC = 8
Byte 3 = 11 01 10 00

Note that 
*/

#define CAN_AS_LOOPBAck true
#define CAN_DEFAULT_CS_PIN 10

namespace Chetch{
    class MCP2515Device : public ArduinoDevice{
        public:
            static  const byte ARDUINO_MESSAGE_SIZE = 16;

            enum CANMessageType{
                CAN_TYPE_NONE = 0,
                CAN_TYPE_ERROR,
                CAN_TYPE_DATA,
                CAN_TYPE_OTHER
            };

            typedef bool (*MessageListener)(MCP2515Device*, byte, ArduinoMessage*);
            typedef bool (*ErrorListener)(MCP2515Device*, int errorCode);

        private:
            MCP2515 mcp2515;
            struct can_frame canInFrame;
            struct can_frame canOutFrame;

            byte nodeID = 0;
            
            MessageListener messageReceivedListener = NULL;
            MessageListener messageSentListener = NULL;
            ErrorListener errorListener = NULL;
            
        public:
            MCP2515Device(byte nodeID = 0, int csPin = CAN_DEFAULT_CS_PIN);
            
            bool begin() override;
            void loop() override;
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;

            void addMessageReceivedListener(MessageListener listener){ messageReceivedListener = listener; }
            void addMessageSentListener(MessageListener listener){ messageSentListener = listener; }
            void addErrorListener(ErrorListener listener){ errorListener = listener; }

            bool sendMessage(ArduinoMessage *message);
    };
} //end namespace