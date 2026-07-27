#include "ChetchPageCycler.h"

namespace Chetch{
    
    PageCycler::PageCycler(byte pin) : SwitchArray(SwitchDevice::SwitchMode::PASSIVE, pin, 2, 20){
        
    }

    PageCycler::~PageCycler(){
        if(pageCount > 0){
            Page* page = currentPage;
            while(page != NULL){
                Page* nextPage = page->next;
                delete page;
            }
        }
    }
   

    bool PageCycler::begin(){
        SwitchArray::begin();
        
        currentPage = firstPage;
        return begun;
    }

     void PageCycler::addPage(Page* page2add){
        if(page2add == NULL)return;

        if(firstPage == NULL){
            pageCount++;
            firstPage = page2add;
            firstPage->pageNumber = pageCount;
            lastPage = page2add;
        } else {
            Page* page = firstPage;
            while(page != NULL){
                if(page->next == NULL){
                    pageCount++;
                    page->next = page2add;
                    page2add->prev = page;
                    page2add->pageNumber = pageCount;
                    lastPage = page2add;
                    page = NULL;
                } else {
                    page = page->next;
                }
            }
        }
     }

    PageCycler::Page* PageCycler::getPage(byte pageNumber){
        if(firstPage != NULL){
            Page* page = firstPage;
            while(page != NULL){
                if(page->pageNumber == pageNumber)return page;
            }
        }
        return NULL;
    }

    void PageCycler::trigger(){
        SwitchArray::trigger();

        //Check first if this is a button release
        bool release = !isOn();
        if(!release)return;

        Page* newPage = currentPage;

        //Forwards or backwards or double press?
        byte pin = getPin();
        if(pin == getFirstPin()){
            //prev pressed
            if(currentPage->prev == NULL){
                newPage = lastPage;
            } else {
                newPage = currentPage->prev;
            }      
        } else {
            //next pressed
            if(currentPage->next == NULL){
                newPage = firstPage;
            } else {
                newPage = currentPage->next;
            }   
        }

        //raiseEvent(EVENT_NEXT_PAGE, currentPageNumber);

        if(pageListener != NULL){
            if(pageListener(this, currentPage, newPage)){
                currentPage = newPage;
            }
        } else {
            currentPage = newPage;
        }
    }
}