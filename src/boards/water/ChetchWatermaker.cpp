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
        setReportInterval(REPORT_INTERVAL_IDLE); //value when not running
        waterMonitorNode = mcp.addNodeDependency(waterMonitorNodeID, 32); //32*32(TIMESTAMP RESOLUTION = 5) = 1024 millis allowed drift before being regarded as stale
        
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

        });

        hps.addSwitchListener([](SwitchDevice* sd,bool on){
            Watermaker* wm = (Watermaker*)sd->Board;

            if(wm->isRunning() && on){
                wm->error(WMErrorCode::HIGH_PRESSURE);
            }

            if(!wm->hasError()){
                wm->updateDisplay();
            }
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

    void Watermaker::loop(){
        CANBusNode::loop();

        if(millis() - waterMonitorLastUpdate > 1500 && waterMonitorPresent){
            waterMonitorPresent = false;
            updateDisplay();
        }
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
        waterProduced = 0.0;

        display.backlight(true, -1);
        updateDisplay();

        setReportInterval(REPORT_INTERVAL_RUNNING);
    }

    void Watermaker::stop(){
        //turn off the various relays
        pressurePump.turn(false);
        feederPump.turn(false);
        
        solenoidSalt.turn(false);
        solenoidFresh.turn(false);
        
        //record data and update display
        currentSession->stoppedOn = millis();

        updateDisplay();
        display.backlight(true, 5000);

        setReportInterval(REPORT_INTERVAL_IDLE);
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
        }
    }

    void Watermaker::updateDisplay(DisplayMode displayMode){
        display.updateDisplay(displayMode);
    }

    void Watermaker::renderWaterMonitor(){
        if(!waterMonitorPresent && false){
            display.setCursor(0, 1);
            display.print("Monitor offline");
        } else {
            char s[20];
            s[0] = 0;
            display.setCursor(0, 1);
            sprintf(s, "TDS: %d (%d.%dC)   ", ppm, (int)temp, (int)((temp - (int)temp) * 10));
            display.print(s);

            display.setCursor(0, 2);
            s[0] = 0;
            sprintf(s, "FR: %d.%d L/M    ", (int)flowRate1, (int)((flowRate1 - (int)flowRate1) * 10));
            display.print(s);
        }
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
            if(isRunning()){
                renderWaterMonitor();

                display.setCursor(0, 3);
                unsigned int duration = (unsigned int)((millis() - currentSession->startedOn) / 1000);
                display.print("Run: ");
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
                    renderWaterMonitor();
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
        //Maybe not call base depending on byte limit
        CANBusNode::setReportInfo(message);

        //Note the byte limit here
        message->add((byte)errorCode);
        message->add((byte)currentMode);
        message->add(isRunning());
        /*unsigned int duration = 0;
        if(isRunning()){
            duration = (unsigned int)((millis() - currentSession->startedOn) / 1000);
        }
        message->add(duration);*/
    }

    void Watermaker::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData){
        CANBusNode::handleReceivedBusMessage(sourceNodeID, message, canData);

        if(sourceNodeID == waterMonitorNode->getNodeID()){
            //We focus on data here
            if(message->type == ArduinoMessage::TYPE_DATA || message->type == ArduinoMessage::TYPE_XDATA){
                waterMonitorLastUpdate = millis();

                if(!waterMonitorPresent){
                    waterMonitorPresent = true;
                    updateDisplay();
                }

                //Serial.print("WMON "); Serial.println(message->sender);
                switch(message->sender){
                    case 10: //TDS
                        message->populate<double, double>(canData);
                        ppm = (int)message->get<double>(1);
                        if(ppm <= 20)ppm = 0;

                        //Serial.print("PPM: ");
                        //Serial.println(ppm);
                        break;

                    case 11: //TEMP
                        message->populate<double>(canData);
                        temp = message->get<double>(0);
                        //Serial.print("Temp: ");
                        //Serial.println(temp);
                        break;

                    case 12: //FLOW RATE
                        message->populate<double>(canData);
                        flowRate1 = message->get<double>(0);

                        //Serial.print("FR: ");
                        //Serial.println(flowRate1);
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