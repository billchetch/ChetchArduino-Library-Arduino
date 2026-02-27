#include "ChetchPageCycler.h"

namespace Chetch{
    
    PageCycler::PageCycler(byte pin, byte maxPages) : SelectorSwitch(SwitchDevice::SwitchMode::PASSIVE, pin, 2){
        this->maxPages = maxPages;
        if(maxPages > 0){
            pages = new Page*[maxPages];
            for(byte i = 0; i < maxPages; i++){
                pages[i] = NULL;
            }
        }
    }

    PageCycler::~PageCycler(){
        if(maxPages > 0){
            delete[] pages;
        }
    }
   

    bool PageCycler::begin(){
        SwitchDevice::begin();
        
        currentPageNumber = 1;
        return begun;
    }

     void PageCycler::addPage(Page* page){
        if(page == NULL)return;

        if(page->number == 0){
            for(byte i = 0; i < maxPages; i++){
                if(pages[i] == NULL)page->number = i + 1;
            }
        }

        if(page->number > 0 && page->number <= maxPages){
            pages[page->number - 1] = page;
        }
     }

    PageCycler::Page* PageCycler::getPage(byte pageNumber){
        if(pageNumber > 0 && pageNumber <= maxPages){
            return pages[pageNumber - 1];
        } else {
            return NULL;
        }
    }



    void PageCycler::trigger(){
        SelectorSwitch::trigger();

        //Forwards or backwards?
        byte prevPage = currentPageNumber;
        byte sp = getSelectedPin();
        if(sp == getFirstPin()){ //backwards
            if(currentPageNumber > 1){
                currentPageNumber--;
            } else {
                currentPageNumber = maxPages;
            }       
        } else if(sp == getFirstPin() + 1) { ///forwards
            if(currentPageNumber < maxPages){
                currentPageNumber++;
            } else {
                currentPageNumber == 1;
            }   
        }

        raiseEvent(EVENT_NEXT_PAGE, currentPageNumber);

        if(pageListener != NULL && prevPage != currentPageNumber){
            pageListener(currentPageNumber, maxPages, getPage(currentPageNumber));
        }
    }
}