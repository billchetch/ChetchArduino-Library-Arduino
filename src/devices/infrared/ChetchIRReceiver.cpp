#include "ChetchUtils.h"
#include "ChetchIRReceiver.h"

namespace Chetch{

	IRReceiver::IRReceiver(byte pin) {
		configure(pin);
	}

	void IRReceiver::configure(byte pin){
        receivePin = pin;
	}

	bool IRReceiver::begin(){
		irReceiver.begin(receivePin);
		return true;
	}

	void IRReceiver::setStatusInfo(ArduinoMessage* message){
        ArduinoDevice::setStatusInfo(message);
        
        message->add(receivePin);
    }

	bool IRReceiver::executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) {
		bool handled  = ArduinoDevice::executeCommand(command, message, response);
		if(handled)return true;

		switch (command) {
			case START:
				irReceiver.resume();
				recording = true;
				response->add(recording);
				break;

			case STOP:
				irReceiver.resume();
				recording = false;
				response->add(recording);
				break;
		}

		return true;
	}

	void IRReceiver::populateOutboundMessage(ArduinoMessage* message, byte messageID) {
		if (messageID == MESSAGE_ID_IRCODERECEIVED) {
			message->type = ArduinoMessage::TYPE_DATA;
			//message->add(irReceiver.decodedIRData.protocol); //Protocol
			//message->add(irReceiver.decodedIRData.address); //Address
			//message->add(irReceiver.decodedIRData.command); //Command
			
			irReceiver.resume(); //ready for next result	
		}
	} 

	void IRReceiver::loop() {
		
		static unsigned long elapsed = 0;
		if ((millis() - elapsed > 100) && irReceiver.decode()) {
			elapsed = millis();
			
			raiseEvent(EVENT_IRCODERECEIVED);
			enqueueMessageToSend(MESSAGE_ID_IRCODERECEIVED);
		}
	}

} //end namespace
