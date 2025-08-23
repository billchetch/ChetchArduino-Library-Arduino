#ifndef CHETCH_ARDUINO_PAGE_CYCLER_H
#define CHETCH_ARDUINO_PAGE_CYCLER_H


#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

#include "devices/ChetchSwitchDevice.h"

namespace Chetch{
    class PageCycler : public SwitchDevice {
        public:
            const byte EVENT_NEXT_PAGE = 100;

            typedef void (*NextPageListener)(byte pageNumber, byte maxPages);

        private:
            byte maxPages = 0;
            byte currentPage = 0;

            NextPageListener pageListener = NULL;

        public:
            PageCycler(byte pin, byte maxPages);
            
            byte getCurrentPage(){ return currentPage; }
            byte getMaxPages(){ return maxPages; }
            void addNextPageListener(NextPageListener listener){ pageListener = listener; }

            bool begin() override;
            void trigger() override;
    }; //end class
} //end namespae
#endif