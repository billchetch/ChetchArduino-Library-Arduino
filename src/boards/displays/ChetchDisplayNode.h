#ifndef CHETCH_DISPLAYNODE_H
#define CHETCH_DISPLAYNODE_H

#include "ChetchArduinoBoard.h"
#include "boards/ChetchCANBusNode.h"

#include "devices/displays/ChetchLCDI2C.h"
#include "devices/displays/ChetchPageCycler.h"

namespace Chetch{

    class DisplayNode : public CANBusNode{
        public:
            static const byte MESSAGE_ID_REQUEST_STATUS = 200;

            class Page : public PageCycler::Page{
                public: 
                    struct DataSource{
                        byte nodeID = 0;
                        byte* requestStatusIDs = NULL;
                        byte requestStatusIDsCount = 0;
                        DataSource* next = NULL;

                        DataSource(byte nodeID, byte* reqStatusIDs = NULL, byte reqStatusIDsCount = 0){
                            this->nodeID = nodeID;
                            if(reqStatusIDs != NULL && reqStatusIDsCount > 0){
                                requestStatusIDsCount = reqStatusIDsCount;
                                requestStatusIDs = new byte[requestStatusIDsCount];
                                for(byte i = 0; i < requestStatusIDsCount; i++){
                                    requestStatusIDs[i] = reqStatusIDs[i];
                                }
                            }
                        }

                        ~DataSource(){
                            if(requestStatusIDs != NULL){
                                delete[] requestStatusIDs;
                                requestStatusIDsCount = 0;
                            }
                        }
                    };

                protected:
                    DisplayNode* board = NULL;
                    DataSource* firstDataSource = NULL;
                        
                public:
                    bool clearBeforeRender = false;
                    bool activateBeforeRender = false;
                    
                protected:
                    bool addDataSource(byte sourceNodeID, byte* requestStatusIDs = NULL, byte requestStatusIDsCount = 0,  byte tolerance = 32){
                        if(board == NULL)return false;

                        DataSource* ds = NULL; 
                        if(firstDataSource == NULL){
                            ds = new DataSource(sourceNodeID, requestStatusIDs, requestStatusIDsCount);
                            firstDataSource = ds;
                            board->addNodeDependency(sourceNodeID, tolerance);
                            return true;
                        } else {
                            DataSource* dsource = firstDataSource;
                            do{
                                if(dsource->nodeID == sourceNodeID){
                                   return false;
                                }
                                if(dsource->next != NULL)dsource = dsource->next;
                            } while(dsource->next != NULL);
                            
                            ds = new DataSource(sourceNodeID, requestStatusIDs, requestStatusIDsCount);
                            dsource->next = ds;
                            board->addNodeDependency(sourceNodeID, tolerance);
                            return true;
                        }
                    }

                public:
                    virtual void initialise(DisplayNode* displayNode){
                        board = displayNode;
                    }
                    DataSource* getFirstDataSource(){ return firstDataSource; }
                    bool isDataSource(byte sourceNodeID){
                        DataSource* ds = firstDataSource;
                        while(ds != NULL){
                            if(ds->nodeID == sourceNodeID){
                                return true;
                            }
                            ds = ds->next;
                        }
                        return false;
                    }
                    virtual void update(DisplayNode* displayNode, byte sourceNodeID, ArduinoMessage* message, byte* canData, byte canDLC){}
                    virtual void render(DisplayNode* displayNode, LCDI2C* display) = 0;
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
            
            void addPage(DisplayNode::Page* page);

            bool onPageChange(DisplayNode::Page* currentPage, DisplayNode::Page* newPage);

            virtual void renderPage(byte updateTag, bool displayInitialised);

            void handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData, byte canDLC) override;

    }; //end class
} //end namespcae
#endif