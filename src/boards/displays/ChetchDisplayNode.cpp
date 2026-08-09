#include "ChetchDisplayNode.h"

namespace Chetch{
    DisplayNode::DisplayNode(byte nodeID, byte serialPin, byte cols, byte rows, LCDI2C::RefreshRate refreshRate, byte pageCyclerPin) : CANBusNode(nodeID, serialPin),
                        display(cols, rows, refreshRate),
                        pageCycler(pageCyclerPin)
     {

        //Add event handlers
        pageCycler.addPageListener([](PageCycler* pageCycler, PageCycler::Page* currentPage, PageCycler::Page* newPage){
            DisplayNode* dn = (DisplayNode*)pageCycler->Board;
            return dn->onPageChange((DisplayNode::Page*)currentPage, (DisplayNode::Page*)newPage);
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
        //Note here you could set filter policy (or in derived class)
        //mcp.setFilterPolicy(MCP2515Device::FilterPolicy::DO_NOT_USE_FILTERS);

        Page* page = (Page*)pageCycler.getFirstPage();
        while(page != NULL){
            page->initialise(this);
            page = (Page*)page->next;
        }

        bool retVal = CANBusNode::begin(io);
        if(retVal){
            //throttle IO
            ((CANBusIO*)getIO())->setThrottle(100);
            
            activate();
            display.updateDisplay();
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
                if(ds->requestStatusIDs != NULL){
                    for(byte i = 0; i < ds->requestStatusIDsCount; i++){
                        getIO()->enqueueMessageToSend(this, MESSAGE_ID_REQUEST_STATUS + ds->nodeID, ds->requestStatusIDs[i]);
                    }
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

    bool DisplayNode::onPageChange(Page* currentPage, Page* newPage){
        if(!isActive()){
            activate();
            currentPage->clearBeforeRender = true;
            display.updateDisplay();
            return false; //cancels assigning current page the new page
        } else {
            activate(); 
            display.updateDisplay();
            newPage->clearBeforeRender = true;
            return true; //proceed making current page the new page
        }
    }

    void DisplayNode::activate(){
        if(!active){
            display.backlight(true);
            lastStatusRequest = 0; //so we immediately request status 
        }
        active = true;
        lastActivityOn = millis();
    }

    void DisplayNode::renderPage(byte updateTag, bool displayInitialised){
        DisplayNode::Page* page = (Page*)pageCycler.getCurrentPage();
        if(page == NULL)return;

        if(page->clearBeforeRender){
            display.clearDisplay();
            page->clearBeforeRender = false;
        }
        if(page->activateBeforeRender){
            activate();
            page->activateBeforeRender = false;
        }
        page->render(this, &display);
        
    }

    void DisplayNode::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData, byte canDLC){
        CANBusNode::handleReceivedBusMessage(sourceNodeID, message, canData, canDLC);

        Page* page = (Page*)pageCycler.getFirstPage();
        while(page != NULL){
            if(page->isDataSource(sourceNodeID)){
                page->update(this, sourceNodeID, message, canData, canDLC);
            }
            if(page == (Page*)pageCycler.getCurrentPage()){
                display.updateDisplay();
            }
            page = (Page*)page->next;
        }
    }

} //end of namespace
