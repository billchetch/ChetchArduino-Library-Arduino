#include "ChetchWatermaker.h"

namespace Chetch{
    Watermaker::Watermaker(byte nodeID, byte serialPin, byte waterMonitorNodeID) : CANBusNode(nodeID, serialPin),
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
        
        
        selector.addSelectListener([](SelectorSwitch* ss, byte selectedPin){
            //Capture this
            Watermaker* wm = (Watermaker*)ss->Board;
            
            //Do the thing
            wm->selectMode((OperationalMode)selectedPin);
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
        });

        hps.addSwitchListener([](SwitchDevice* sd,bool on){
            Watermaker* wm = (Watermaker*)sd->Board;

            if(wm->isRunning() && on){
                wm->error(WMErrorCode::HIGH_PRESSURE);
            }
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

    void Watermaker::loop(){
        CANBusNode::loop();

        if(millis() - waterMonitorLastUpdate > 1500 && waterMonitorPresent){
            waterMonitorPresent = false;
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

        setReportInterval(REPORT_INTERVAL_IDLE);
    }

    void Watermaker::reset(){
        error(WMErrorCode::NO_ERROR);  
    }

    void Watermaker::error(WMErrorCode ec){  
        if(isRunning()){
            stop();
        }

        errorCode = ec;
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

    void Watermaker::handleReceivedBusMessage(byte sourceNodeID, ArduinoMessage* message, byte* canData, byte canDLC){
        CANBusNode::handleReceivedBusMessage(sourceNodeID, message, canData, canDLC);

        if(sourceNodeID == waterMonitorNode->getNodeID()){
            //We focus on data here
            if(message->type == ArduinoMessage::TYPE_DATA || message->type == ArduinoMessage::TYPE_XDATA){
                waterMonitorLastUpdate = millis();

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