#ifndef CHETCH_DISPLAYNODE_H
#define CHETCH_DISPLAYNODE_H

#include "ChetchArduinoBoard.h"
#include "boards/ChetchCANBusNode.h"

#include "devices/displays/ChetchLCDI2C.h"
#include "devices/displays/ChetchPageCycler.h"


namespace Chetch{

    class DisplayNode : public CANBusNode{
        public:

            class Page : public PageCycler::Page{
                protected: 
                    struct DataSource{
                        byte nodeID = 0;
                        byte senderID = 0;
                        DataSource* next = NULL;
                    };

                    DataSource* firstDataSource = NULL;
                    
                    LCDI2C* display = NULL;
                    DisplayNode* board = NULL;

                protected:
                    bool addDataSource(byte sourceNodeID, byte senderID, byte tolerance = 32){
                        if(board == NULL)return false;

                        DataSource* ds = NULL; 
                        if(firstDataSource == NULL){
                            ds = new DataSource;
                            ds->nodeID = sourceNodeID;
                            ds->senderID = senderID;
                            firstDataSource = ds;
                            board->addNodeDependency(sourceNodeID, tolerance);
                            return true;
                        } else {
                            DataSource* dsource = firstDataSource;
                            do{
                                if(dsource->nodeID == sourceNodeID && dsource->senderID == senderID){
                                   return false;
                                }
                                if(dsource->next != NULL)dsource = dsource->next;
                            } while(dsource->next != NULL);
                            ds = new DataSource;
                            ds->nodeID = sourceNodeID;
                            ds->senderID = senderID;
                            dsource->next = ds;
                            board->addNodeDependency(sourceNodeID, tolerance);
                            return true;
                        }
                    }

                public:
                    virtual void initialise(DisplayNode* displayNode){
                        board = displayNode;
                        display = &board->display;
                    }
                    bool canRender(){ return display != NULL; }
                    bool isDataSource(byte sourceNodeID, byte senderID){
                        DataSource* ds = firstDataSource;
                        while(ds != NULL){
                            if(ds->nodeID == sourceNodeID && ds->senderID == senderID){
                                return true;
                            }
                            ds = ds->next;
                        }
                        return false;
                    }
                    virtual void update(DisplayNode* displayNode, byte sourceNodeID, ArduinoMessage* message, byte* canData){}
                    virtual void render() = 0;
            };
            
        private:
            unsigned long lastActivityOn = 0;
            bool active = false;
            unsigned int sleepTimeout = 5000;
            unsigned long lastStatusRequest = 0;
            unsigned int requestStatusInterval = 5000;

        public:
            //Devices
            LCDI2C display;
            PageCycler pageCycler;

        public:
            DisplayNode(byte nodeID, byte serialPin, byte cols, byte rows, LCDI2C::RefreshRate refreshRate, byte pageCyclerPin);

            bool begin(MessageIO* io = NULL) override; //will return false if fails to begin
            void loop() override;
            void populateOutboundMessage(ArduinoMessage* message, byte messageID) override;

            void setSleepAfter(unsigned int sleepTimeout){ this->sleepTimeout = sleepTimeout; }
            bool isActive(){ return active; }
            void activate();
            void updateDisplay(bool clear, byte updateTag = 0);

            void addPage(DisplayNode::Page* page);
            
            virtual void renderPage(byte updateTag, bool displayInitialised);

            void handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData) override;

    }; //end class
} //end namespcae
#endif