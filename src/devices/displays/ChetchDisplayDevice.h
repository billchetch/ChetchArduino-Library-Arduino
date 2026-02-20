#ifndef CHETCH_ARDUINO_DISPLAY_DEVICE_H
#define CHETCH_ARDUINO_DISPLAY_DEVICE_H


#include <Arduino.h>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
    template <typename T>
    class DisplayDevice : public ArduinoDevice {
        public:
            enum RefreshRate
            {
                NO_REFRESH = 0,
                REFRESH_1HZ = 1000,
                REFRESH_2HZ = 500,
                REFRESH_10HZ = 100,
                REFRESH_20HZ = 50,
                REFRESH_50Hz = 20

            };          

            enum DisplayPreset{
                CLEAR = 0,
                BOARD_STATS,
                HELLO_WORLD
            };

            typedef bool (*DisplayHandler)(byte tag, bool displayInitialised); 

        private:
            T pDisplay; //Should be a pointer to the display

            unsigned int rows = 0;
            unsigned int cols = 0;

            unsigned int lockDuration = 0;
            unsigned long lockedAt = 0;
            
            DisplayHandler displayHandler = NULL;
            RefreshRate refreshRate = RefreshRate::REFRESH_50Hz;
            unsigned long lastUpdated = 0;
            bool update = false;
            byte updateTag = 0;
            byte lastUpdateTag = 0;
            unsigned long requestInitialiseOn = 0;

        protected: 
            

        public:
            DisplayDevice(T pDisplay, unsigned int rows = 0, unsigned int cols = 0, RefreshRate refreshRate = RefreshRate::REFRESH_50Hz){ 
                this->pDisplay = pDisplay; 
                setDimensions(rows, cols);
                this->refreshRate = refreshRate;
            } 

            virtual void initialiseDisplay() = 0;
            virtual bool isDisplayConnected() = 0;
            
            void addDisplayHandler(DisplayHandler handler){ displayHandler = handler; }
            
            bool begin() override{
                initialiseDisplay();
                begun = true; 
                return begun;
            }

            void loop() override{
                ArduinoDevice::loop();
                
                //Lock has ended
                if(isLocked() && millis() - lockedAt > lockDuration){
                    clearDisplay();
                    unlock();
                }

                static bool displayInitialised = true;    
                if(requestInitialiseOn > 0){
                    if(millis() - requestInitialiseOn > 500){
                        initialiseDisplay();
                        requestInitialiseOn = 0;
                        displayInitialised = true;    
                        lastUpdated = millis();
                    }
                } else {
                    if(displayHandler != NULL && !isLocked() && update && (millis()- lastUpdated > (int)refreshRate)){
                        //we check connected before we try and display
                        if(!isDisplayConnected()){
                            requestInitialiseOn = millis();
                            if(requestInitialiseOn == 0)requestInitialiseOn = 1;
                        } else {
                            if(displayHandler(updateTag, displayInitialised)){
                                update = false;
                                lastUpdated = millis();
                                lastUpdateTag = updateTag;
                                updateTag = 0;
                            }
                            displayInitialised = false;
                        }
                    }
                }
            }

            bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override{
                bool handled = ArduinoDevice::executeCommand(command, message, response);
        
                if(!handled)
                {
                    
                }
                return handled;
            }

            void setStatusInfo(ArduinoMessage* message) override{
                ArduinoDevice::setStatusInfo(message);
                
                message->add(rows);
                message->add(cols);
                message->add(refreshRate);
            }

            void lock(unsigned int lockFor){
                //set to 0 to remove lock, lockFor in ms
                lockDuration = lockFor;
                lockedAt = millis();
            }
            void unlock(){ lockDuration = 0; }
            bool isLocked(){ return lockDuration > 0; }

            T getDisplay(){ return pDisplay; }
            
            void updateDisplay(byte tag = 0){
                update = true;
                updateTag = tag; 
            }

            byte getLastUpdateTag(){ return lastUpdateTag; }
            
            void setDimensions(unsigned int rows, unsigned int cols){
                this->rows = rows;
                this->cols = cols;
            }

            virtual void clearDisplay() = 0;

            virtual void clearLine(unsigned int ln){
                if(isLocked())return;
                setCursor(0, ln);
                for(unsigned int i = 0; i < cols; i++){
                    print<char>(' ');
                }
                setCursor(0, ln);
            }
            
            void setCursor(unsigned int cx, unsigned int cy){ 
                if(isLocked())return;
                getDisplay()->setCursor(cx, cy); 
            }

            template <typename S> void print(S s){ getDisplay()->print(s); }

            //too common not to shorthand
            void printLine(char* s, unsigned int ln = 0){
                setCursor(0, ln);
                print(s);
                int n = min(cols - strlen(s), 0);
                for(int i = 0; i < n; i++){
                    print<char>(' ');
                }
            }
    }; //end class
} //end namespae
#endif