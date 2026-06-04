#include "ChetchUtils.h"
#include "ChetchArduinoBoard.h"
#include "ChetchLCDI2C.h"


namespace Chetch{
    LCDI2C::LCDI2C(byte cols, byte rows, RefreshRate refreshRate) : DisplayDevice(&lcd, rows, cols, refreshRate), lcd(DEFAULT_I2C_ADDRESS, cols, rows) {
        //empty
    }
    
    void LCDI2C::initialiseDisplay(){
        if(begun){
            Wire.end();
            delay(1);
        }
        
        Wire.begin();
        Wire.setClock(400000L);
        Wire.setWireTimeout(25000, false);

        if(begun){
            delay(1);
        }
        lcd.init();
        backlight(false);
        if(begun){
            delay(1);
        }
    }

    bool LCDI2C::isDisplayConnected(){
        if(Wire.getWireTimeoutFlag() != 0){
            Wire.clearWireTimeoutFlag();
        }
        Wire.beginTransmission(DEFAULT_I2C_ADDRESS); // Start transmission to the I2C addres
        bool transmitSuccess = Wire.endTransmission() == 0;
        return transmitSuccess && (Wire.getWireTimeoutFlag() == 0);
    }

    void LCDI2C::backlight(bool on, int onFor){
        if(on){
            lcd.backlight();
            backlightOn = millis();
            backlightOnFor = onFor;
        } else {
            lcd.noBacklight();
            backlightOnFor = -1;
        }
    }

    /*bool LCDI2C::begin(){
       
        //display.noBacklight();
        begun = true;
        return begun;
	}*/

    void LCDI2C::loop(){
        DisplayDevice::loop();

        if(backlightOnFor > 0 && millis() - backlightOn > backlightOnFor){
            backlight(false);
        }
        //Do stuff here
    }

    bool LCDI2C::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response){
        bool handled = DisplayDevice::executeCommand(command, message, response);
        
        if(!handled)
        {
            switch(command){
                case ArduinoDevice::DeviceCommand::ON:
                    backlight(true, message->getArgumentCount() > 1 ? message->get<int>(1) : -1);
                    handled = true;
                    break;

                case ArduinoDevice::DeviceCommand::OFF:
                    backlight(false);
                    handled = true;
                    break;

            }
        }
        return handled;
    }

    void LCDI2C::clearDisplay(){
        lcd.clear();
    }

}
