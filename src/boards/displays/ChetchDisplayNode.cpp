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
                dn->renderPage((DisplayNode::Page*)currentPage);
                return false; //cancels assigning current page the new page
            } else {
                dn->activate(); //to keep this alive
                dn->renderPage((DisplayNode::Page*)newPage);
                return true; //proceed making current page the new page
            }
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
            renderPage((DisplayNode::Page*)pageCycler.getCurrentPage());
        }
        return retVal;
    }

    void DisplayNode::loop(){
        CANBusNode::loop();

        if(active && millis() - lastActivityOn > sleepTimeout){
            display.backlight(false);
            active = false;
        }
    }

    void DisplayNode::activate(){
        if(!active){
            display.backlight(true);
        }
        active = true;
        lastActivityOn = millis();

    }

    void DisplayNode::renderPage(DisplayNode::Page* page){
        if(page == NULL)return;

        page->render();
    }

    void DisplayNode::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData){
        CANBusNode::handleReceivedBusMessage(sourceNodeID, message, canData);

        
        Page* page = (Page*)pageCycler.getFirstPage();
        while(page != NULL){
            page->update(sourceNodeID, message, canData);
            page = (Page*)page->next;
        }
    }

} //end of namespace
