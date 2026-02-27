#ifndef CHETCH_ARDUINO_PAGE_CYCLER_H
#define CHETCH_ARDUINO_PAGE_CYCLER_H


#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/ChetchSelectorSwitch.h"

namespace Chetch{
    class PageCycler : public SelectorSwitch {
        public:
            const byte EVENT_NEXT_PAGE = 100;
            const byte EVENT_PREV_PAGE = 100;

            struct Page{
                byte number = 0;

                virtual ~Page(){};
            };

            typedef void (*PageListener)(byte pageNumber, byte maxPages, PageCycler::Page* page);


        private:
            byte maxPages = 0;
            byte currentPageNumber = 0;

            PageListener pageListener = NULL;

            Page** pages = NULL;

        public:
            PageCycler(byte pin, byte maxPages);
            ~PageCycler();
            
            byte getMaxPages(){ return maxPages; }
            byte getCurrentPageNumber(){ return currentPageNumber; }
            void addPageListener(PageListener listener){ pageListener = listener; }
            void addPage(PageCycler::Page* page);
            PageCycler::Page* getPage(byte pageNumber);
            PageCycler::Page* getCurrentPage(){ return getPage(currentPageNumber); }
            
            bool begin() override;
            void trigger() override;


    }; //end class
} //end namespae
#endif