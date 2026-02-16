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
                REFRESH_10HZ = 100,
                REFRESH_50Hz = 20
            };          

            enum DisplayPreset{
                CLEAR = 0,
                BOARD_STATS,
                HELLO_WORLD
            };

            typedef bool (*DisplayHandler)(byte tag); 

        private:
            T pDisplay; //Should be a pointer to the display

            unsigned int lockDuration = 0;
            unsigned long lockedAt = 0;
            
            DisplayHandler displayHandler = NULL;
            RefreshRate refreshRate = RefreshRate::REFRESH_50Hz;
            unsigned long lastUpdated = 0;
            bool update = false;
            byte updateTag = 0;

        public:
            DisplayDevice(T pDisplay, RefreshRate refreshRate = RefreshRate::REFRESH_50Hz){ 
                this->pDisplay = pDisplay; 
                this->refreshRate = refreshRate;
            } 

            void addDisplayHandler(DisplayHandler handler){ displayHandler = handler; }
            void loop() override{
                ArduinoDevice::loop();
                
                if(isLocked() && millis() - lockedAt > lockDuration){
                    clearDisplay();
                    unlock();
                }

                if(displayHandler != NULL && !isLocked() && update && (millis()- lastUpdated > (int)refreshRate)){
                    if(displayHandler(updateTag)){
                        update = false;
                        lastUpdated = millis();
                        updateTag = 0;
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
            
            virtual void clearDisplay() = 0;
            
            void setCursor(unsigned int cx, unsigned int cy){ 
                if(isLocked())return;
                getDisplay()->setCursor(cx, cy); 
            }

            template <typename S> void print(S s){ getDisplay()->print(s); }

            //too common not to shorthand
            void print(char* s){ print<char*>(s); }
    }; //end class
} //end namespae
#endif