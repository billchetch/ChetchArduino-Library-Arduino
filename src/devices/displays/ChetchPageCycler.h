#ifndef CHETCH_ARDUINO_PAGE_CYCLER_H
#define CHETCH_ARDUINO_PAGE_CYCLER_H


#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/ChetchSwitchArray.h"

namespace Chetch{
    class PageCycler : public SwitchArray {
        public:
            const byte EVENT_NEXT_PAGE = 101;
            const byte EVENT_PREV_PAGE = 100;
            const byte EVENT_DOUBLE_PRESS = 102;

            class Page{
                public: 
                    Page* next = NULL;
                    Page* prev = NULL;

                    byte pageNumber = 0;

                public:
                    virtual ~Page(){};
            };

            typedef bool (*PageListener)(PageCycler* pageCycler, PageCycler::Page* currentPage, PageCycler::Page* newPage);


        private:
            byte pageCount = 0;
            
            PageListener pageListener = NULL;

            Page* currentPage = NULL;
            Page* firstPage = NULL;
            Page* lastPage = NULL;

        public:
            PageCycler(byte pin);
            ~PageCycler();
            
            byte getPageCount(){ return pageCount; }
            PageCycler::Page* getFirstPage(){ return firstPage; }
            PageCycler::Page* getCurrentPage(){ return currentPage; }
            void addPageListener(PageListener listener){ pageListener = listener; }
            void addPage(PageCycler::Page* page);
            PageCycler::Page* getPage(byte pageNumber);
            
            bool begin() override;
            void trigger() override;


    }; //end class
} //end namespae
#endif