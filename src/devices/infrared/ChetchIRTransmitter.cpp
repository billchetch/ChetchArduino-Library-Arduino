#include "ChetchUtils.h"
#include "ChetchIRTransmitter.h"


namespace Chetch{

	IRTransmitter::IRTransmitter(byte pin) {
		configure(pin);
	}

	void IRTransmitter::configure(byte pin){
        transmitPin = pin;
    }

	bool IRTransmitter::begin(){
		irSender.begin(transmitPin);
		return true;
	}

	void IRTransmitter::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        
        message->add(transmitPin);
    }

	void IRTransmitter::loop() {
		ArduinoDevice::loop();

		if (sendFlag) {
			if (millis() - lastSend < SEND_INTERVAL_THRESHOLD)return;

			switch (protocol) {
				case SAMSUNG: //17
					//irSender.sendSamsung(address, command, 0);
					break;

				default:
					break;

			} //end protocol switch

			sendFlag = false;
			lastSend = millis();
		}
		
		if (repeatFlag) {
			switch (protocol) {
				case SAMSUNG: //17
					if ( millis() - lastSend >= 60) {
						//irSender.sendSamsungRepeat();
						//irSender.sendSamsungLGRepeat(); //As of version 3.9.0
						lastSend = millis();
					}
					break;
			}
		}
	}

	bool IRTransmitter::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) {
		bool handled = ArduinoDevice::executeCommand(command, message, response);
        if(handled)return true;

		switch (command) {
			case SEND:
				//protocol = message->argumentAsUInt(getArgumentIndex(message, MessageField::PROTOCOL));
				//address = message->argumentAsUInt(getArgumentIndex(message, MessageField::ADDRESS));
				bool repeat = false; //message->argumentAsBool(getArgumentIndex(message, MessageField::USE_REPEAT));
				if (repeatFlag && !repeat) { //so this is an end to repeating so we just set flags to false
					sendFlag = false;
					repeatFlag = false;
				}
				else { //otherwise here is a normal send or a send with repeat
					switch (protocol) {
					case SAMSUNG: //17
						sendFlag = true;
						break;

					default:
						setErrorInfo(response, ERROR_UNRECOGNISED_PROTOCOL);
						return false;
					}
					repeatFlag = repeat;
				}
				
				response->add(protocol);
				response->add(address);
				response->add(command);
				response->add(sendFlag);
				response->add(repeatFlag);
				handled = true;
				break;

		} //end command  switch
			
		return handled;
	}
} //end namespace
