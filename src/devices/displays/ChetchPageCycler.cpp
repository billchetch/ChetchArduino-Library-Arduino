#include "ChetchPageCycler.h"

namespace Chetch{
    
    PageCycler::PageCycler(byte pin, byte maxPages) : SwitchDevice(SwitchDevice::SwitchMode::PASSIVE, pin, 100){
        //empty
    }
   

    bool PageCycler::begin(){
        SwitchDevice::begin();
        
        currentPage = 1;
        return begun;
    }

    void PageCycler::trigger(){
        SwitchDevice::trigger();

        //we cycle the page
        if(currentPage >= maxPages){
            currentPage = 1;
        } else {
            currentPage++;
        }

        raiseEvent(EVENT_NEXT_PAGE);

        if(pageListener != NULL){
            pageListener(currentPage, maxPages);
        }
    }
}