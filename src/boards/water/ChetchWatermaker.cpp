#include "ChetchWatermaker.h"

namespace Chetch{
    Watermaker::Watermaker(byte nodeID, byte serialPin, byte waterMonitorNodeID) : CANBusNode(nodeID, serialPin),
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
        //Board stuff
        setReportInterval(REPORT_INTERVAL*10); //value when not running
        this->waterMonitorNodeID = waterMonitorNodeID;
        mcp.addNodeDependency(waterMonitorNodeID);
        
        //Devices
        //Legacy stuff, in newer boards this is set to 7 to allow for use of altserialsoft library
        mcp.setIndicatorPin(9);
        
        //Add event handlers to devices
        display.setReportInterval(DISPLAY_UPDATE_INTERVAL); //Setting report interval allows for an interval (rather than direct call) based update
        display.addEventListener([](ArduinoDevice* device, byte eventID, byte eventTag){
            Watermaker* wm = (Watermaker*)device->Board;
            if(eventID == ArduinoDevice::EVENT_REPORT_READY){
                if(wm->isRunning()){
                    wm->updateDisplay(DisplayMode::RUNNING);
                }
            }
            return false;
        });
        display.addDisplayHandler([](ArduinoDevice* dd, byte updateTag, bool displayInitialised){
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
                    wm->error(WMErrorCode::LOW_PRESSURE);
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
                wm->error(WMErrorCode::HIGH_PRESSURE);
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
        //Display
        addDevice(&display);

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

    bool Watermaker::begin(MessageIO* io){
        bool retVal = CANBusNode::begin(io);
        if(retVal){
            display.backlight(true, 5000);
            updateDisplay(DisplayMode::NORMAL);
        }
        return retVal;
    }

    bool Watermaker::isRunning(){
        return feederPump.isOn();
    }

    bool Watermaker::hasError(){
        return errorCode != WMErrorCode::NO_ERROR;
    }

    void Watermaker::selectMode(OperationalMode operationalMode){
        if(isRunning()){
            stop();
        }

        if(hasError()){
            reset();
        }
        
        currentMode = operationalMode;
        currentSession = &sessions[currentMode - OperationalMode::MAKE_WATER];

        updateDisplay(DisplayMode::NORMAL);

        display.backlight(true, 5000);
    }

    void Watermaker::start(){
        if(currentMode == NOT_SET || hasError())return;
        
        if(feederPump.isOn()){
            error(WMErrorCode::FP_INCORRECT);
            return;
        }

        if(pressurePump.isOn()){
            error(WMErrorCode::PP_INCORRECT);
            return;
        }

        if(lps.isOn()){
            error(WMErrorCode::LPS_INCORRECT);
            return;
        }

        if(hps.isOn()){
            error(WMErrorCode::HPS_INCORRECT);
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
            default:
                break;
        }

        feederPump.turn(true);

        currentSession->startedOn = millis();
        currentSession->count++;

        display.backlight(true, -1);
        updateDisplay();

        setReportInterval(REPORT_INTERVAL);
    }

    void Watermaker::stop(){
        pressurePump.turn(false);
        feederPump.turn(false);
        
        solenoidSalt.turn(false);
        solenoidFresh.turn(false);
        
        currentSession->stoppedOn = millis();

        updateDisplay();
        display.backlight(true, 5000);

        setReportInterval(REPORT_INTERVAL*10);
    }

    void Watermaker::reset(){
        error(WMErrorCode::NO_ERROR);  
    }

    void Watermaker::error(WMErrorCode ec){  
        if(isRunning()){
            stop();
        }

        bool changed = ec != errorCode;
        errorCode = ec;
        if(hasError() && changed){
            display.backlight(true, -1);
            updateDisplay(DisplayMode::ERROR);

            ArduinoMessage* msg = mcp.getMessageForBoard(ArduinoMessage::MessageType::TYPE_ERROR);
            msg->add((byte)50); //TODO: this isa board custom error value
            msg->add((byte)errorCode);
            mcp.sendMessage(msg);
        }
    }

    void Watermaker::updateDisplay(DisplayMode displayMode){
        display.updateDisplay(displayMode);
    }

    bool Watermaker::renderDisplay(DisplayMode displayMode, bool displayInitialised){
        if(displayInitialised){
            display.clearDisplay();
        }

        if(hasError() || displayMode == DisplayMode::ERROR){
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print(">>>> ERROR: ");
            display.print(errorCode);
            display.print(" <<<<");
        } else if(displayMode == DisplayMode::RUNNING && !displayInitialised) {
            //this is called at regular intervals while running
            display.setCursor(0, 3);
            if(isRunning()){
                unsigned int duration = (unsigned int)((millis() - currentSession->startedOn) / 1000);
                display.print("Running: ");
                display.print(duration);
                display.print("s   ");
            }
        } else {
            if(displayMode == DisplayMode::DISPLAY_MODE_NOT_SET){
                displayMode = lastDisplayMode;
            } else {
                lastDisplayMode = displayMode;
            }


            display.clearDisplay();

            //Line 0: Display selection
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

                default:
                    break;
            }
            
            //Lines 1 and 2
            switch(displayMode){
                case DisplayMode::DIAGNOSTIC:
                    display.backlight(true, -1);

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
                    break;

                case DisplayMode::NORMAL:
                    if(!waterMonitorPresent){
                        display.setCursor(0, 1);
                        display.print("Monitor offline");
                    } else {
                        display.setCursor(0, 1);
                        display.print("TDS: ");
                        display.print(ppm);
                        display.print(" (");
                        display.print(temp);
                        display.print("C)");
                        
                        display.setCursor(0, 2);
                        display.print("FR: ");
                        display.print(flowRate1);
                        display.print(" L/M");
                    }
                    break;

                default:
                    //Do nothing
                    break;
            }

            //Line 3 running history
            if(!isRunning()){
                display.setCursor(0, 3);
                if(currentSession->count != 0){
                    unsigned int duration = (unsigned int)((currentSession->stoppedOn - currentSession->startedOn) / 1000);
                    display.print("Run#:");
                    display.print(currentSession->count);
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

    void Watermaker::setReportInfo(ArduinoMessage* message){
        message->add((byte)errorCode);
        message->add((byte)currentMode);
        message->add(isRunning());
        unsigned int duration = 0;
        if(isRunning()){
            duration = (unsigned int)((millis() - currentSession->startedOn) / 1000);
        }
        message->add(duration);
    }

    void Watermaker::onReportReady(){
        mcp.sendMessageForBoard(MESSAGE_ID_REPORT);
    }

    void Watermaker::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData){
        if(sourceNodeID == waterMonitorNodeID){
            //We focus on data here
            if(message->type == ArduinoMessage::TYPE_DATA){
                if(!waterMonitorPresent){
                    waterMonitorPresent = true;
                    updateDisplay();
                }

                //Serial.print("WMON "); Serial.println(message->sender);
                switch(message->sender){
                    case 10: //TDS
                        message->populate<double, double>(canData);
                        ppm = message->get<double>(1);
                        break;

                    case 11: //TEMP
                        message->populate<double>(canData);
                        temp = message->get<double>(0);
                        break;

                    case 12: //FLOW RATE
                        message->populate<double>(canData);
                        flowRate1 = message->get<double>(0);
                        break;

                    default:
                        break;
                }
            }
        } else {
            //some kind of error this
            /*Serial.print("HRM: "); 
            Serial.print(sourceNodeID);   
            Serial.print(" ");   
            Serial.println(message->type);*/
        }
    }
}