#ifndef CHETCH_MCP2515_DEVICE_H
#define CHETCH_MCP2515_DEVICE_H

#include <Arduino.h>

#include <ChetchArduinoBoard.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>


#include <SPI.h>
#include <mcp2515.h>

#define CAN_AS_LOOPBACK false
//#define CAN_AS_LOOPBACK true
#define CAN_DEFAULT_INDICATOR_PIN 7 //leave 8 and 9 for Software Serial/AltSerial soft
//#define COUNT_ERROR_CODES 12 //Comment out if not counting error codes

/*
NANO
INT PIN = D2 (this is not required if looping)
CS PIN = D10 (this can be altered mcp)
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
- Byte 1 = Timestamp


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
D7: TX2IF (CANINTF[4]) Transmit Buffer-2-Empty Interrupt Flag bit
D6: TXREQ (TXB2CNTRL[3]) Buffer 2, Message-Transmit-Request bit
D5: TX1IF (CANINTF[3]) Transmit Buffer-1-Empty Interrupt Flag bit
D4: TXREQ (TXB1CNTRL[3]) Buffer 1, Message-Transmit-Request bit
D3: TX0IF (CANINTF[2]) Transmit Buffer-0-Empty Interrupt Flag bit
D2: TXREQ (TXB0CNTRL[3]) Buffer 0, Message-Transmit-Request bit
D1: RX1IF (CANINTF[1]) Receive-Buffer-1-Full Interrupt Flag
D0: RX0IF (CANINTF[0]) Receive-Buffer-0-Full Interrupt Flag

*/


namespace Chetch{
    class MCP2515Device : public ArduinoDevice{
        public:
            static const byte ARDUINO_MESSAGE_SIZE = 22; //8 possible arguments each of 1 byte (cos CanFrame is 8 bytes)
            
            static const byte MAX_NODE_ID = 15;
            static const byte MIN_NODE_ID = 1;
            
            static const uint32_t MESSAGE_TYPES_MASK = 0x18000000; //Singles out bits 5 and 4 of the message type portion of CAN ID (byte 4)
            static const uint32_t NODE_MASK = 0x00F00000; //Singles out bits related to the node ID part of the CAN ID (byte 3)

            static const unsigned int DEFAULT_PRESENCE_INTERVAL = 5000;
            static const byte TIMESTAMP_RESOLUTION = 4; //Shift right by this many bits... lower number makes finer resolution
            static const unsigned int INDICATOR_INTERVAL = 50;

            enum MCP2515ErrorCode : byte{
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
                CUSTOM_ERROR, //For individual applications or when send it cancelled
                DEBUG_ASSERT, //For debug purposes
            };

            enum ResetRegime : byte{
                NOT_SET = 0,
                RESET_DEVICE,
                RESET_ERRORS,
                RESET_ALL,
            };

            enum IndicateMode : byte{
                NO_INDICATOR = 0,
                INDICATE_ON_SEND,
                INDICATE_ON_RECIEVE,
                INDICATE_FULL
            };
            
            //Node Dependency
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

                    unsigned long getLastUpdated(){ return updatedOn; }

                    bool isUpdated(){ return updated; }

                    void reset(){
                        nodeTime = 0;
                        updatedOn = 0;
                        updated = false;
                    }

                    void setNodeTime(unsigned long remoteMillis, unsigned int remoteElapsed){
                        if(updated){
                            unsigned int localElapsed = (unsigned int)(millis() - updatedOn);
                            if(localElapsed >= remoteElapsed){
                                nodeTime = remoteMillis + (localElapsed - remoteElapsed);
                            } else {
                                nodeTime = remoteMillis - (remoteElapsed - localElapsed);
                            }
                        } else {
                            nodeTime = remoteMillis;
                            updated = true;
                        }
                        
                        updatedOn = millis();
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
    
            typedef void (*MessageListener)(MCP2515Device*, byte, ArduinoMessage*, can_frame*); //device, node, message, canData
            typedef void (*ErrorListener)(MCP2515Device*, MCP2515ErrorCode, unsigned long errorData);
            typedef bool (*SendValidator)(MCP2515Device*, ArduinoMessage*, unsigned long canID, byte* canData);

            MCP2515 mcp2515; //should be moved to private
            ArduinoMessage imsg; //shuuld be moved to private
            ArduinoMessage omsg; //shuuld be moved to private

        private:
            bool initialised = false;
            
            byte indicatorPin = CAN_DEFAULT_INDICATOR_PIN;
            bool indicated = false;
            unsigned long indicatedOn = 0;

            IndicateMode indicateMode = INDICATE_ON_SEND;

        public: //SHOULD BE PRIVATE/PROTECTED
            unsigned int presenceInterval = 0; //how often to broadcast a PRESENCE message
            unsigned long lastPresenceOn = 0;
            unsigned int presenceSentCount = 0; //to indicate first presence (successfully) sent note this will be zero on rollover

            byte responseID = 0;
            bool remoteInitialised = false;
            bool pinged = false;
            bool remoteReset = false;

            //REMVOE! for debug only this
            unsigned int statusRequestCount = 0;
            unsigned int statusResponseCount = 0;
            
            
        protected:
            byte nodeID = 0;
            
            NodeDependency* firstDependency = NULL; 

            MessageListener messageReceivedListener = NULL;
            SendValidator sendValidator = NULL;
            ErrorListener errorListener = NULL;

            bool broadcastError = false;
            MCP2515ErrorCode lastError = MCP2515ErrorCode::NO_ERROR;
            unsigned long lastErrorData = 0;
            
#if defined(COUNT_ERROR_CODES)
            byte errorCounts[COUNT_ERROR_CODES];
#endif
            unsigned int errorCodeFlags = 0;
            unsigned long lastErrorOn = 0;
            
        public: //temp:  should be protected this
            struct can_frame canInFrame;
            struct can_frame canOutFrame;
            
        protected:
            void init(bool forceInit = false); //Must be called after construtor but before configuring stuff hence why it's present in config type methods
            byte crc5(byte* data, byte len);
            bool vcrc5(byte crc, byte* data, byte len);
            
        public:
            MCP2515Device(byte nodeID, int csPin, unsigned int presenceInterval);
            ~MCP2515Device();

            byte getNodeID(){ return nodeID; }
            
            void resetErrors();
            int clearReceive();
            bool allowSending();

            //Node dependency
            NodeDependency* addNodeDependency(byte nodeID, byte tolerance = 1);
            bool hasDependencies(){ return firstDependency != NULL; }
            NodeDependency* getDependency(byte nodeID);
        
            
            void raiseError(MCP2515ErrorCode errorCode, unsigned long errorData = 0);
#if defined(COUNT_ERROR_CODES)
            byte* getErrorCounts(){ return errorCounts; }
#endif

            void indicate(bool on, bool force = false);
            bool canIndicate(IndicateMode mode){ return (mode & indicateMode) == mode; }
            void setIndicatorPin(byte pin){ indicatorPin = pin; }
            void setIndicateMode(IndicateMode mode){ indicateMode = mode; }
            
            bool begin() override;
            void loop() override;
            void setStatusInfo(ArduinoMessage* response) override;
            
            void addMessageReceivedListener(MessageListener listener){ messageReceivedListener = listener; }
            void addSendValidator(SendValidator validator){ sendValidator = validator; }
            void addErrorListener(ErrorListener listener){ errorListener = listener; }

            ArduinoMessage* getMessageForHandler(byte handlerID, ArduinoMessage::MessageType messageType, byte tag = 0);
            ArduinoMessage* getMessageForDevice(ArduinoDevice* device, ArduinoMessage::MessageType messageType = ArduinoMessage::TYPE_DATA, byte tag = 0);
            ArduinoMessage* getMessageForBoard(ArduinoMessage::MessageType messageType = ArduinoMessage::TYPE_DATA, byte tag = 0);
            
            MCP2515ErrorCode sendMessageForDevice(ArduinoDevice* device, byte messageID);
            MCP2515ErrorCode sendMessageForBoard(byte messageID);
            MCP2515ErrorCode sendMessage(ArduinoMessage *message, bool raiseError = true);
            virtual void onMessageSent(ArduinoMessage *message){};

            bool checkReceive();
            void readMessage();
            void handleReceivedMessage(byte sourceNodeID, ArduinoMessage *message);

            virtual void onMessageReceived(byte sourceNodeID, ArduinoMessage *message);
    };
} //end namespace
#endif //end prevent multiple inclusions