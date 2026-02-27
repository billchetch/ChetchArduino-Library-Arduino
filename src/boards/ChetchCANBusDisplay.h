#ifndef CHETCH_CAN_BUS_DISPLAY_H
#define CHETCH_CAN_BUS_DISPLAY_H

#include "ChetchArduinoBoard.h"
#include "boards/ChetchCANBusNode.h"
#include "devices/displays/ChetchPageCycler.h"


namespace Chetch{

    template <typename T>
    class CANBusDisplay : public CANBusNode{
        public:
            
        public:
            T pDisplay;
            PageCycler pageCycler;
            
        public:
            CANBusDisplay(byte nodeID, byte serialPin, T pDisplay, byte pageCyclerPin, byte maxPages) : CANBusNode(nodeID, serialPin),
                                pageCycler(pageCyclerPin, maxPages)
            {
                this->pDisplay = pDisplay;


                pDisplay->addDisplayHandler([](T dd, byte updateTag, bool displayInitialised){
                    CANBusDisplay* cbd = (CANBusDisplay*)dd->Board;
                    return cbd->renderPage(cbd->pageCycler.getCurrentPageNumber(), cbd->pageCycler.getCurrentPage(), updateTag, displayInitialised);
                });
                
                pageCycler.addPageListener([](PageCycler* pc, byte currentPage, byte maxPages){
                    CANBusDisplay* cbd = (CANBusDisplay*)pc->Board;
                    cbd->pDisplay->updateDisplay(0);
                });
                
                addDevice(pDisplay);
                addDevice(&pageCycler);
            }

            virtual bool renderPage(byte pageNumber, PageCycler::Page* page, byte updateTag, bool displayInitailised) = 0;
    };
} //end namespace
#endif

