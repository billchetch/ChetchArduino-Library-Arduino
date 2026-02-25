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

            //Send message out
            wm->mcp.sendMessageForDevice(sd, SwitchDevice::MESSAGE_ID_TRIGGERED);
        });

        hps.addSwitchListener([](SwitchDevice* sd,bool on){
            Watermaker* wm = (Watermaker*)sd->Board;

            if(wm->isRunning() && on){
                wm->error(ErrorCode::HIGH_PRESSURE);
            }

            if(!wm->hasError()){
                //display.updateDisplay(0);
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

        //display.updateDisplay(DisplayMode::STARTED);
    }

    void Watermaker::stop(){
        pressurePump.turn(false);
        feederPump.turn(false);
        
        solenoidSalt.turn(false);
        solenoidFresh.turn(false);
        
        stoppedOn = millis();
        //display.updateDisplay(DisplayMode::STOPPED);
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
}