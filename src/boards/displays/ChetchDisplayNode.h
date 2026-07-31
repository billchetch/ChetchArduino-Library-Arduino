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
                    LCDI2C* display = NULL;


                public:
                    virtual void initialise(DisplayNode* displayNode){
                        display = &displayNode->display;
                    }
                    bool canRender(){ return display != NULL; }
                    virtual void update(DisplayNode* displayNode, byte sourceNodeID, ArduinoMessage* message, byte* canData){}
                    virtual void render() = 0;
            };

            
        private:
            unsigned long lastActivityOn = 0;
            bool active = false;
            unsigned int sleepTimeout = 5000;

        public:
            //Devices
            LCDI2C display;
            PageCycler pageCycler;

        public:
            DisplayNode(byte nodeID, byte serialPin, byte cols, byte rows, LCDI2C::RefreshRate refreshRate, byte pageCyclerPin);

            bool begin(MessageIO* io = NULL) override; //will return false if fails to begin
            void loop() override;

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