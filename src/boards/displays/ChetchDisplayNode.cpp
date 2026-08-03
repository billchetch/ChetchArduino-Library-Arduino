#include "ChetchDisplayNode.h"

namespace Chetch{
    DisplayNode::DisplayNode(byte nodeID, byte serialPin, byte cols, byte rows, LCDI2C::RefreshRate refreshRate, byte pageCyclerPin) : CANBusNode(nodeID, serialPin),
                        display(cols, rows, refreshRate),
                        pageCycler(pageCyclerPin)
     {

        //Add event handlers
        pageCycler.addPageListener([](PageCycler* pageCycler, PageCycler::Page* currentPage, PageCycler::Page* newPage){
            DisplayNode* dn = (DisplayNode*)pageCycler->Board;
            
            if(!dn->isActive()){
                dn->activate();
                dn->updateDisplay(false);
                return false; //cancels assigning current page the new page
            } else {
                dn->activate(); 
                dn->updateDisplay(true);
                return true; //proceed making current page the new page
            }
        });

        display.addDisplayHandler([](ArduinoDevice* dd, byte updateTag, bool displayInitialised){
            DisplayNode* dn = (DisplayNode*)dd->Board;
            dn->renderPage(updateTag, displayInitialised);
            return true;
        });

        //Add devices
        addDevice(&display); //ID = 10
        addDevice(&pageCycler); //ID = 11
    }

    void DisplayNode::addPage(DisplayNode::Page* page){
        pageCycler.addPage(page);
    }

    bool DisplayNode::begin(MessageIO* io){
        Page* page = (Page*)pageCycler.getFirstPage();
        while(page != NULL){
            page->initialise(this);
            page = (Page*)page->next;
        }

        bool retVal = CANBusNode::begin(io);
        if(retVal){
            activate();
            updateDisplay(false);
            //renderPage((DisplayNode::Page*)pageCycler.getCurrentPage());
        }
        return retVal;
    }

    void DisplayNode::loop(){
        CANBusNode::loop();

        if(sleepTimeout > 0 && active && millis() - lastActivityOn > sleepTimeout){
            display.backlight(false);
            active = false;
        }

        if(requestStatusInterval > 0 && millis() - lastStatusRequest > requestStatusInterval){
            lastStatusRequest = millis();
            Page* page = (Page*)pageCycler.getCurrentPage();
            Page::DataSource* ds = page->getFirstDataSource();
            while(ds != NULL){
                if(ds->requestStatus){
                    Serial.print("RS for ");
                    Serial.print(ds->nodeID);
                    Serial.print(" ");
                    Serial.println(ds->senderID);
                    getIO()->enqueueMessageToSend(this, MESSAGE_ID_REQUEST_STATUS + ds->nodeID, ds->senderID);
                }
                ds = ds->next;
            }
        }
    }

    void DisplayNode::populateOutboundMessage(ArduinoMessage* message, byte messageID){
        CANBusNode::populateOutboundMessage(message, messageID);

        if(messageID >= MESSAGE_ID_REQUEST_STATUS){
            byte nodeID = messageID - MESSAGE_ID_REQUEST_STATUS;
            byte senderID = message->tag;

            message->type = ArduinoMessage::MessageType::TYPE_STATUS_REQUEST;
            message->tag = 0;
            if(senderID < ArduinoBoard::START_DEVICE_IDS_AT){
                message->sender = 0;
            } else {
                message->sender = 1 + (senderID - ArduinoBoard::START_DEVICE_IDS_AT);
            }
            message->add(nodeID);
        }
    }

    void DisplayNode::activate(){
        if(!active){
            display.backlight(true);
        }
        active = true;
        lastActivityOn = millis();

    }

    void DisplayNode::updateDisplay(bool clear, byte updateTag){
        if(clear){
            display.clearDisplay();
        }
        display.updateDisplay(updateTag);
    }

    void DisplayNode::renderPage(byte updateTag, bool displayInitialised){
        DisplayNode::Page* page = (Page*)pageCycler.getCurrentPage();
        if(page == NULL)return;

        if(page->canRender())page->render();
    }

    void DisplayNode::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData){
        CANBusNode::handleReceivedBusMessage(sourceNodeID, message, canData);

        Page* page = (Page*)pageCycler.getFirstPage();
        while(page != NULL){
            if(page->isDataSource(sourceNodeID, message->sender)){
                page->update(this, sourceNodeID, message, canData);
                if(page == (Page*)pageCycler.getCurrentPage() && isActive()){
                    updateDisplay(false);
                }
            }
            page = (Page*)page->next;
        }
    }

} //end of namespace
