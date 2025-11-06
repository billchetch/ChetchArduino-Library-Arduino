#ifndef CHETCH_MCP2515_DEVICE_H
#define CHETCH_MCP2515_DEVICE_H

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
The key to this is using the extended ID (so 29 bit) ID value as follows (reading left to right with 4 on the left):

- Byte 4 = First 5 bits are the message type (allowing the type to determin priority)
- Byte 3 = Node and sender 4 bits + 4 bits so 16 nodes and each node can have 16 senders (allowing node and device to determin priority)
- Byte 2 = Tag and CRC: 3 bits for tag and 5 bits for CRC which ic calculated over the data and is provided to guard against SPI issues mainly)
- Byte 1 = Message structure:  2 bits for argument count, then 2 bits for arg 1 length, 2 bits for arg2 length and 2 bits for arg 3 length. Argument 4 length is inferred.  Note finally bit values are all 1 less than the intended value.

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
(Source: MCP2515-Stand-Alone-CAN-Controller-with-SPI-20001801J.pdf)
D0: RX0IF (CANINTF[0]) Receive-Buffer-0-Full Interrupt Flag
D1: RX1IF (CANINTF[1]) Receive-Buffer-1-Full Interrupt Flag
D2: TXREQ (TXB0CNTRL[3]) Buffer 0, Message-Transmit-Request bit
D3: TX0IF (CANINTF[2]) Transmit Buffer-0-Empty Interrupt Flag bit
D4: TXREQ (TXB1CNTRL[3]) Buffer 1, Message-Transmit-Request bit
D5: TX1IF (CANINTF[3]) Transmit Buffer-1-Empty Interrupt Flag bit
D6: TXREQ (TXB2CNTRL[3]) Buffer 2, Message-Transmit-Request bit
D7: TX2IF (CANINTF[4]) Transmit Buffer-2-Empty Interrupt Flag bit
*/

#define CAN_AS_LOOPBACK false
#define CAN_DEFAULT_CS_PIN 10
#define CAN_DEFAULT_INDICATOR_PIN 9

namespace Chetch{
    class MCP2515Device : public ArduinoDevice{
        public:
            static const byte ARDUINO_MESSAGE_SIZE = 16;
            static const byte MASTER_NODE_ID = 1;
            static const byte MAX_NODE_ID = 15;
            static const byte MIN_NODE_ID = 1;
            static const int NO_FILTER = -1;
            static const unsigned int PRESENCE_INTERVAL = 2000;
            static const byte TIMESTAMP_RESOLUTION = 4; //Shift right by this many bits... lower number makes finer resolution
            static const unsigned int INDICATOR_INTERVAL = 50;

            static const byte EVENT_READTY_TO_SEND = 1;

            enum MCP2515ErrorCode{
                NO_ERROR = 0,
                UNKNOWN_RECEIVE_ERROR, //RX error
                UNKNOWN_SEND_ERROR, //TX error
                NO_MESSAGE, //RX error (but fires when readMessage is called and nothing is on the buffer)
                INVALID_MESSAGE, //Message format error
                FAIL_TX, //TX error
                ALL_TX_BUSY, //TX error
                READ_FAIL, //RX error
                CRC_ERROR, //RX error - probably from SPI issues but who knows.. 
                STALE_MESSAGE, //RX error - an old message
                SYNC_ERROR, //RX error - if presence is out of sync
                CUSTOM_ERROR, //For individual applications
                DEBUG_ASSERT, //For debug purposes
            };
            
            class NodeDependency{
                public: //NOTE: change to private
                    byte nodeID = 0;
                    unsigned long nodeTime = 0;
                    unsigned long updatedOn = 0;
                    bool updated = false;
                    byte tolerance = 1;

                public:
                    NodeDependency* next = NULL;

                public: 
                    NodeDependency(byte nid, byte tolerance = 1){
                        nodeID = nid;
                        this->tolerance = tolerance;
                    }

                    byte getNodeID(){ return nodeID; }

                    void reset(){
                        nodeTime = 0;
                        updatedOn = 0;
                        updated = 0;
                    }

                    bool inSync(unsigned long ms, unsigned int msElapsed){
                        if(!updated)return true;

                        unsigned long ent = getEstimatedNodeTime();

                        unsigned int tol = msElapsed / 200; //allow for drift of 0.5% according to specs
                        if(ent > ms){
                            return ent - ms <= tol;
                        } else {
                            return ms - ent <= tol;
                        }
                    }

                    bool setNodeTime(unsigned long ms, unsigned int msElapsed){
                        bool sync = inSync(ms, msElapsed);
                        
                        nodeTime = ms;
                        updatedOn = millis();
                        updated = true;
                        
                        return sync;
                    }

                    unsigned long getEstimatedNodeTime(){
                        if(!updated)return 0;

                        return (millis() - updatedOn) + nodeTime;
                    }

                    int getDiff(byte timestamp, byte resolution = TIMESTAMP_RESOLUTION){
                        byte estimatedTimestamp = (byte)((getEstimatedNodeTime() >> resolution) & 0xFF);

                        int diff = abs((int)timestamp - (int)estimatedTimestamp);
                        diff = min(diff, 256 - diff);
                        return diff;
                    }

                    bool isStale(byte timestamp, byte resolution = TIMESTAMP_RESOLUTION){
                        if(!updated)return false;

                        return getDiff(timestamp, resolution) > tolerance;
                    }
            };

            typedef void (*MessageListener)(MCP2515Device*, byte, ArduinoMessage*, byte*); //device, node, message, canData
            typedef void (*CommandListener)(MCP2515Device*, byte, ArduinoDevice::DeviceCommand, ArduinoMessage*);
            typedef void (*ErrorListener)(MCP2515Device*, MCP2515ErrorCode, unsigned long errorData);
            typedef bool (*SendValidator)(MCP2515Device*, ArduinoMessage*, unsigned long canID, byte* canData);

            MCP2515 mcp2515; //should be moved to private
            ArduinoMessage amsg; //shuuld be moved to private

        private:
            bool initialised = false;
            
            byte nodeID = 0;
            byte indicatorPin = CAN_DEFAULT_INDICATOR_PIN;
            bool indicated = false;
            unsigned long indicatedOn = 0;

            NodeDependency* firstDependency = NULL;

            

            byte maskNum = 0; //max is 1
            byte filterNum = 0; //max is 5 after regNum == 1 then maskNum increments

        public: //SHOULD BE PRIVATE
            unsigned int presenceInterval = 0; //how often to broadcast a PRESENCE message
            unsigned long lastPresenceOn = 0;
            bool presenceSent = false; //to indicate first presence sent

            bool statusRequested = false;
            bool pinged = false;

        protected:
            MessageListener messageReceivedListener = NULL;
            CommandListener commandListener = NULL;
            SendValidator sendValidator = NULL;
            ErrorListener errorListener = NULL;

            MCP2515ErrorCode lastError = MCP2515ErrorCode::NO_ERROR;
            unsigned long lastErrorData = 0;
            byte errorCounts[12];
            unsigned int errorCodeFlags = 0;
            unsigned long lastErrorOn = 0;
            

            bool canSend = false; //is set to true if either a message is received or a certain period has elapsed
            
        public: //temp:  should be protected this
            struct can_frame canInFrame;
            struct can_frame canOutFrame;
            
        private:
            void init(); //Must be called after construtor but before configuring stuff hence why it's present in config type methods
            
        protected:
            byte crc5(byte* data, byte len);
            bool vcrc5(byte crc, byte* data, byte len);
            
        public:
            MCP2515Device(byte nodeID = 0, unsigned int presenceInterval = PRESENCE_INTERVAL, int csPin = CAN_DEFAULT_CS_PIN);
            ~MCP2515Device();

            byte getNodeID(){ return nodeID; }
            void reset();
            bool begin() override;
            virtual bool allowSending();
            bool addNodeDependency(byte nodeID, byte tolerance = 1);
            NodeDependency* getDependency(byte nodeID);
            
            virtual void raiseError(MCP2515ErrorCode errorCode, unsigned long errorData = 0);
            byte* getErrorCounts(){ return errorCounts; }

            void indicate(bool on, bool force = false);
            void loop() override;
            void setStatusInfo(ArduinoMessage* response) override;
            void setPingInfo(ArduinoMessage* response) override;
            
            void addMessageReceivedListener(MessageListener listener){ messageReceivedListener = listener; }
            void addCommandListener(CommandListener listener){ commandListener = listener; }
            void addSendValidator(SendValidator validator){ sendValidator = validator; }
            void addErrorListener(ErrorListener listener){ errorListener = listener; }

            ArduinoMessage* getMessageForDevice(ArduinoDevice* device, ArduinoMessage::MessageType messageType = ArduinoMessage::TYPE_DATA, byte tag = 0);
            virtual bool sendMessage(ArduinoMessage *message);
            void readMessage();
            virtual void handleReceivedMessage(byte sourceNodeID, ArduinoMessage *message);

            uint32_t createFilterMask(bool checkNodeID, bool checkMessageType, bool checkSender);
            uint32_t createFilter(int nodeID, int messageType, int sender);
            bool addFilter(uint32_t mask, uint32_t filter);
    };
} //end namespace
#endif //end prevent multiple inclusions