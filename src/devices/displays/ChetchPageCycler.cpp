#include "ChetchPageCycler.h"

namespace Chetch{
    
    PageCycler::PageCycler(byte pin, byte maxPages) : SelectorSwitch(SwitchDevice::SwitchMode::PASSIVE, pin, 2){
        this->maxPages = maxPages;
    }
   

    bool PageCycler::begin(){
        SwitchDevice::begin();
        
        currentPage = 1;
        return begun;
    }

    void PageCycler::trigger(){
        SelectorSwitch::trigger();

        //Forwards or backwards?
        byte prevPage = currentPage;
        byte sp = getSelectedPin();
        if(sp == getFirstPin()){ //backwards
            if(currentPage > 1){
                currentPage--;
            } else {
                currentPage = maxPages;
            }       
        } else if(sp == getFirstPin() + 1) { ///forwards
            if(currentPage < maxPages){
                currentPage++;
            } else {
                currentPage == 1;
            }   
        }

        raiseEvent(EVENT_NEXT_PAGE, currentPage);

        if(pageListener != NULL && prevPage != currentPage){
            pageListener(currentPage, maxPages);
        }
    }
}