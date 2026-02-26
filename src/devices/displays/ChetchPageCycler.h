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

            typedef void (*PageListener)(byte pageNumber, byte maxPages);

        private:
            byte maxPages = 0;
            byte currentPage = 0;

            PageListener pageListener = NULL;

        public:
            PageCycler(byte pin, byte maxPages);
            
            byte getCurrentPage(){ return currentPage; }
            byte getMaxPages(){ return maxPages; }
            void addPageListener(PageListener listener){ pageListener = listener; }

            bool begin() override;
            void trigger() override;
    }; //end class
} //end namespae
#endif