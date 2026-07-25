#include "ChetchDisplayNode.h"

namespace Chetch{
    DisplayNode::DisplayNode(byte nodeID, byte serialPin, byte cols, byte rows, LCDI2C::RefreshRate refreshRate, byte pageCyclerPin) : CANBusNode(nodeID, serialPin),
                        display(cols, rows, refreshRate),
                        pageCycler(pageCyclerPin, 10)
     {
        //Add event handlers
        pageCycler.addPageListener([](PageCycler* pageCycler, byte currentPageNumber, byte newPageNumber){
            DisplayNode* dn = (DisplayNode*)pageCycler->Board;
            
            if(!dn->isActive()){
                dn->activate();
                return false;
            } else {
                dn->activate(); //to keep this alive
                dn->display.setCursor(0, 1);
                dn->display.print("Page: ");
                dn->display.print(newPageNumber);
                return true;
            }
        });

        //Add devices
        addDevice(&display); //ID = 10
        addDevice(&pageCycler); //ID = 11
    }

    bool DisplayNode::begin(MessageIO* io){
        bool retVal = CANBusNode::begin(io);
        if(retVal){
            activate();
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

} //end of namespace
