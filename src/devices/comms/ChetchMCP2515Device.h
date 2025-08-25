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
The key to this is using the extended ID (so 29 bit) ID value as follows (reading left to right with 1 on the left):

- Byte 1 = First 4 bits specify message priority
- Byte 2 = Type and Tag: 5 bits for Type, 3 bits for Tag
- Byte 3 = Node and Sender: 4 bits for Node (can Node), 5 bits for sender (arduino device) .. effectively a location this
- Byte 4 = Message structure:  2 bits for argument count, then 2 bits for arg 1 length, 2 bits for arg2 length and 2 bits for arg 3 length. Argument 4 length is inferred.  Note finally bit values are all 1 less than the intended value.

More on the message structure... in conjuntion with the can frame DLC value, Byte 3 allows for 4 possible arguments with each argument being of max 4 bytes. 
This corresponds to arduino basic types (float, int, long, byte etc.) It also allows for a varilable length argument if there is only 1 arg as we can use the DLC
value to determine

Free single argument example:
DLC = 0 to 8
Byte = 00 00 00 00 : so the first two bits are 00 => 1 argument and the length is provided by the DLC value

2 arguments example (4 bytes and 2 bytes)
DLC = 6
Byte  = 01 (11 01 00)

3 arguments example (3 bytes and 4 bytes 1 byte)
DLC = 8
Byte = 10 (11 01 00)

4 arguments example (2 bytes and 3 bytes 1 byte 2 byte)
DLC = 8
Byte = 11 (01 10 00) (note 2 bytes is inferred as the last argument as 8 - (2 + 3 + 1) = 2)

ERROR FLAGS (Bits in the byte read from the EFLG register)

e.g. from autowp library there are these defined flags
EFLG_RX1OVR = (1<<7),
EFLG_RX0OVR = (1<<6),
EFLG_TXBO   = (1<<5),
EFLG_TXEP   = (1<<4),
EFLG_RXEP   = (1<<3),
EFLG_TXWAR  = (1<<2),
EFLG_RXWAR  = (1<<1),
EFLG_EWARN  = (1<<0)

D7: RX1OVRF (Receive Buffer 1 Overflow Flag):
    Set when a new valid message is received in Receive Buffer 1, but the buffer is already full.
D6: RX0OVRF (Receive Buffer 0 Overflow Flag):
    Set when a new valid message is received in Receive Buffer 0, but the buffer is already full.
D5: TXBO (Bus-Off Flag):
    Set when the Transmit Error Counter (TEC) exceeds 255, indicating that the device has entered a Bus-Off state.
D4: TXBP (Transmit Error Passive Flag):
    Set when the TEC exceeds 127, indicating that the device has entered an Error Passive state for transmission.
D3: RXBP (Receive Error Passive Flag):
    Set when the Receive Error Counter (REC) exceeds 127, indicating that the device has entered an Error Passive state for reception.
D2: TXWAR (Transmit Error Warning Flag):
    Set when the TEC exceeds 96, indicating a warning level for transmit errors.
D1: RXWAR (Receive Error Warning Flag):
    Set when the REC exceeds 96, indicating a warning level for receive errors.
D0: EWARN (Error Warning Flag):
    Set when either TXWAR or RXWAR is set, providing a general error warning indication

STATUS FLAGS (Bits in the byte read from the ? register)

D7: Transmit Buffer-2-Empty Interrupt Flag bit
D6: Buffer 2, Message-Transmit-Request bit
D5: Transmit Buffer-1-Empty Interrupt Flag bit
D4: Buffer 1, Message-Transmit-Request bit
D3: Transmit Buffer-0-Empty Interrupt Flag bit
D2: Buffer 0, Message-Transmit-Request bit
D1: Receive-Buffer-1-Full Interrupt Flag
D0: Receive-Buffer-0-Full Interrupt Flag
*/

#define CAN_AS_LOOPBACK false
#define CAN_DEFAULT_CS_PIN 10
#define CAN_DEFAULT_INDICATOR_PIN 9
#define CAN_FORWARD_MESSAGES true
#define CAN_REPORT_ERRORS true


namespace Chetch{
    class MCP2515Device : public ArduinoDevice{
        public:
            static const byte ARDUINO_MESSAGE_SIZE = 16;
            static const byte MAX_NODE_ID = 15;
            static const byte MIN_NODE_ID = 1;
            static const int NO_FILTER = -1;
            
            static const byte EVENT_READTY_TO_SEND = 1;

#if CAN_FORWARD_MESSAGES 
            static const byte MESSAGE_ID_FORWARD_RECEIVED = 100;
            static const byte MESSAGE_ID_FORWARD_SENT = 101;
            static const byte MESSAGE_ID_READY_TO_SEND = 102;
#endif
#if CAN_REPORT_ERRORS 
            static const byte MESSAGE_ID_REPORT_ERROR = 110;
#endif


            enum MCP2515ErrorCode{
                NO_ERROR = 0,
                UNKNOWN_RECEIVE_ERROR,
                UNKNOWN_SEND_ERROR,
                NO_MESSAGE,
                INVALID_MESSAGE,
                FAIL_TX,
                ALL_TX_BUSY,
                READ_FAIL
            };
            
            enum CANMessagePriority{
                CAN_PRIORITY_RANDOM = 0,
                CAN_PRIORITY_CRITICAL,
                CAN_PRIORITY_HIGH,
                CAN_PRIORITY_NORMAL,
                CAN_PRIORITY_LOW
            };

            typedef bool (*MessageListener)(MCP2515Device*, CANMessagePriority, byte, ArduinoMessage*);
            typedef bool (*ErrorListener)(MCP2515Device*, MCP2515ErrorCode errorCode);

            MCP2515 mcp2515;
            
        private:
            struct can_frame canInFrame;
            struct can_frame canOutFrame;

            bool initialised = false;
            MCP2515ErrorCode lastError = MCP2515ErrorCode::NO_ERROR;

            byte nodeID = 0;
            byte indicatorPin = CAN_DEFAULT_INDICATOR_PIN;
            bool canSend = false; //is set to true if either a message is received or a certain period has elapsed

            ArduinoMessage amsg;
#if CAN_FORWARD_MESSAGES 
            ArduinoMessage frecvmsg;
            ArduinoMessage fsendmsg;
#endif

            MessageListener messageReceivedListener = NULL;
            MessageListener messageSentListener = NULL;
            ErrorListener errorListener = NULL;

            byte maskNum = 0; //max is 1
            byte filterNum = 0; //max is 5 after regNum == 1 then maskNum increments
            
        private:
            void init(); //Must be called after construtor but before configuring stuff hence why it's present in config type methods
            
        public:
            MCP2515Device(byte nodeID = 0, int csPin = CAN_DEFAULT_CS_PIN);
            
            byte getNodeID(){ return nodeID; }
            void reset();
            bool begin() override;
            void allowSending();
            void raiseError(MCP2515ErrorCode errorCode);
            void indicate(bool on);
            void loop() override;
            
#if CAN_FORWARD_MESSAGES || CAN_REPORT_ERRORS
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;
            void setStatusInfo(ArduinoMessage* response) override;
#endif
            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;
            
            void addMessageReceivedListener(MessageListener listener){ messageReceivedListener = listener; }
            void addMessageSentListener(MessageListener listener){ messageSentListener = listener; }
            void addErrorListener(ErrorListener listener){ errorListener = listener; }

            ArduinoMessage* getMessageForDevice(ArduinoDevice* device, ArduinoMessage::MessageType messageType = ArduinoMessage::TYPE_DATA, byte tag = 0);
            bool sendMessage(ArduinoMessage *message, CANMessagePriority messagePriority = CANMessagePriority::CAN_PRIORITY_RANDOM);
            void readMessage();

            uint32_t createFilterMask(bool checkNodeID, bool checkMessageType, bool checkSender);
            uint32_t createFilter(int nodeID, int messageType, int sender);
            bool addFilter(uint32_t mask, uint32_t filter);
    };
} //end namespace