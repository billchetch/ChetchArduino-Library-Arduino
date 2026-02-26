#include "ChetchWatermaker.h"

namespace Chetch{
    Watermaker::Watermaker(byte nodeID, byte serialPin) : CANBusNode(nodeID, serialPin),
                        display(LCD_COLS, LCD_ROWS, LCD_REFRESH),
                        selector(SwitchDevice::SwitchMode::PASSIVE, SELECTOR_FIRST_PIN, SELECTION_SIZE, SWITCH_TOLERANCE, LOW),
                        startButton(SwitchDevice::SwitchMode::PASSIVE, START_BUTTON_PIN, SWITCH_TOLERANCE, OUTPUT_ONSTATE),
                        lps(SwitchDevice::SwitchMode::PASSIVE, LPS_PIN, SWITCH_TOLERANCE, LOW),
                        hps(SwitchDevice::SwitchMode::PASSIVE, HPS_PIN, SWITCH_TOLERANCE, LOW),
                        solenoidSalt(SwitchDevice::SwitchMode::ACTIVE, SALT_WATER_SOLENOID_PIN, SWITCH_TOLERANCE, OUTPUT_ONSTATE),
                        solenoidFresh(SwitchDevice::SwitchMode::ACTIVE, FRESH_WATER_SOLENOID_PIN, SWITCH_TOLERANCE, OUTPUT_ONSTATE),
                        feederPump(SwitchDevice::SwitchMode::ACTIVE, FEEDER_PUMP_PIN, SWITCH_TOLERANCE, OUTPUT_ONSTATE),
                        pressurePump(SwitchDevice::SwitchMode::ACTIVE, PRESSURE_PUMP_PIN, SWITCH_TOLERANCE, OUTPUT_ONSTATE)

    {
        //Add event handlers to devices
        display.addDisplayHandler([](LCDI2C* dd, byte updateTag, bool displayInitialised){
            Watermaker* wm = (Watermaker*)dd->Board;
            return wm->renderDisplay((DisplayMode)updateTag, displayInitialised);
        });

        
        selector.addSelectListener([](SelectorSwitch* ss, byte selectedPin){
            //Capture this
            Watermaker* wm = (Watermaker*)ss->Board;
            
            //Do the thing
            wm->selectMode((OperationalMode)selectedPin);

            //Send message out for this device
            wm->mcp.sendMessageForDevice(ss, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });

        startButton.addSwitchListener([](SwitchDevice* sd, bool on){
            Watermaker* wm = (Watermaker*)sd->Board;
            if(on){
                wm->start();
            } else {
                wm->stop();
            }

            //Send message out
            wm->mcp.sendMessageForDevice(sd, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });

        lps.addSwitchListener([](SwitchDevice* sd, bool on){
            Watermaker* wm = (Watermaker*)sd->Board;

            if(wm->getCurrentMode() != OperationalMode::EXPEL_AIR && wm->isRunning()){
                if(on){
                    wm->pressurePump.turn(true);
                } else {
                    //possible pressure drop!
                    wm->error(ErrorCode::LOW_PRESSURE);
                }
            }

            if(!wm->hasError()){
                wm->updateDisplay();
            }

            //Send message out
            wm->mcp.sendMessageForDevice(sd, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });

        hps.addSwitchListener([](SwitchDevice* sd,bool on){
            Watermaker* wm = (Watermaker*)sd->Board;

            if(wm->isRunning() && on){
                wm->error(ErrorCode::HIGH_PRESSURE);
            }

            if(!wm->hasError()){
                wm->updateDisplay();
            }

            wm->mcp.sendMessageForDevice(sd, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });

        feederPump.addSwitchListener([](SwitchDevice* sd, bool on){
            Watermaker* wm = (Watermaker*)sd->Board;
            wm->mcp.sendMessageForDevice(sd, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });

        pressurePump.addSwitchListener([](SwitchDevice* sd, bool on){
            Watermaker* wm = (Watermaker*)sd->Board;
            wm->mcp.sendMessageForDevice(sd, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });

        solenoidSalt.addSwitchListener([](SwitchDevice* sd, bool on){
            Watermaker* wm = (Watermaker*)sd->Board;
            wm->mcp.sendMessageForDevice(sd, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });

        solenoidFresh.addSwitchListener([](SwitchDevice* sd, bool on){
            Watermaker* wm = (Watermaker*)sd->Board;
            wm->mcp.sendMessageForDevice(sd, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });
        

        //Add devices to board

        //Inputs
        addDevice(&selector);
        addDevice(&startButton);
        addDevice(&lps);
        addDevice(&hps);

        //Outputs
        addDevice(&solenoidSalt);
        addDevice(&solenoidFresh);
        addDevice(&feederPump);
        addDevice(&pressurePump);
    }

    bool Watermaker::isRunning(){
        return feederPump.isOn();
    }

    bool Watermaker::hasError(){
        return errorCode != ErrorCode::NO_ERROR;
    }

    void Watermaker::selectMode(OperationalMode operationalMode){
        if(isRunning()){
            stop();
        }

        if(hasError()){
            reset();
        }
        
        currentMode = operationalMode;

        updateDisplay(DisplayMode::CHANGE_OPERATIONAL_MODE);
    }

    void Watermaker::start(){
        if(currentMode == NOT_SET || hasError())return;
        
        if(feederPump.isOn()){
            error(ErrorCode::FP_INCORRECT);
            return;
        }

        if(pressurePump.isOn()){
            error(ErrorCode::PP_INCORRECT);
            return;
        }

        if(lps.isOn()){
            error(ErrorCode::LPS_INCORRECT);
            return;
        }

        if(hps.isOn()){
            error(ErrorCode::HPS_INCORRECT);
            return;
        }

        switch(currentMode){
            case MAKE_WATER:
                solenoidSalt.turn(true);
                solenoidFresh.turn(false);
                break;

            case RINSE:
            case EXPEL_AIR:
                solenoidFresh.turn(true);
                solenoidSalt.turn(false);
                break;
        }

        feederPump.turn(true);
        startedOn = millis();
        runCount++;

        updateDisplay(DisplayMode::STARTED);
    }

    void Watermaker::stop(){
        pressurePump.turn(false);
        feederPump.turn(false);
        
        solenoidSalt.turn(false);
        solenoidFresh.turn(false);
        
        stoppedOn = millis();
        updateDisplay(DisplayMode::STOPPED);
    }

    void Watermaker::reset(){
        error(ErrorCode::NO_ERROR);  
    }

    void Watermaker::error(ErrorCode ec){  
        if(isRunning()){
            stop();
        }
        //Serial.print("Error: "); Serial.println(ec);
        errorCode = ec;
        if(hasError()){
            //display.updateDisplay(DisplayMode::ERROR);
        }
    }

    void Watermaker::updateDisplay(DisplayMode displayMode){
        display.updateDisplay(displayMode);
    }

    bool Watermaker::renderDisplay(DisplayMode displayMode, bool displayInitialised){
        if(displayInitialised){
            display.clearDisplay();
        }

        if(hasError()){
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print(">>>> ERROR: ");
            display.print(errorCode);
            display.print(" <<<<");

        } else if(displayMode == DisplayMode::RUNNING && !displayInitialised) {
            //this is called at regular intervals while running
            display.setCursor(0, 3);
            if(isRunning()){
                unsigned int duration = (unsigned int)((millis() - startedOn) / 1000);
                display.print("Running: ");
                display.print(duration);
                display.print("s   ");
            }
        } else {
            display.clearDisplay();

            //Display selection
            display.setCursor(0, 0);
            display.print("Mode: ");
            switch(currentMode){
                case MAKE_WATER:
                display.print("Buat");
                break;

                case RINSE:
                display.print("Bilas");
                break;

                case EXPEL_AIR:
                display.print("Buang");
                break;
            }
            
            display.setCursor(0, 1);
            display.print("FP/PP/SL/ST: ");
            display.print(feederPump.isOn());
            display.print(" ");
            display.print(pressurePump.isOn());
            display.print(" ");
            display.print(solenoidSalt.isOn());
            display.print(" ");
            display.print(solenoidFresh.isOn());

            display.setCursor(0, 2);
            display.print("LPS/HPS: ");
            display.print(lps.isOn());
            display.print(" ");
            display.print(hps.isOn());

            if(!isRunning()){
                display.setCursor(0, 3);
                if(runCount != 0){
                    unsigned int duration = (unsigned int)((stoppedOn - startedOn) / 1000);
                    display.print("Run#:");
                    display.print(runCount);
                    display.print(" for ");
                    display.print(duration);
                    display.print("s   ");
                } else {
                    display.print("Not yet run!");
                }
            }
        }

        return true;
    }
}