#include <Arduino.h>
#define USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE
#include <IRremote.hpp>
#include <ChetchArduinoDevice.h>
#include <ChetchArduinoMessage.h>

namespace Chetch{
	struct IRSendData{
		unsigned int protocol;
		unsigned int address;
		unsigned int command;
	};

	class IRTransmitter : public ArduinoDevice {
	public:
		//in millis the time allowed between sends ... a shorter period send is ignored as the ADM should call and set repeat flag
		static const unsigned int SEND_INTERVAL_THRESHOLD = 250; 

		//Errors
		static const byte ERROR_UNRECOGNISED_PROTOCOL = 100;

		//Events
		static const byte EVENT_IRCODESENT = 1;

	private:
		IRsend irSender;
		byte transmitPin = 0; 
		bool sendFlag = false;
		unsigned long lastSend = 0;
		IRSendData ir2send;

	public:
		IRTransmitter(byte transmitPin);
		
		void configure(byte pin);
		bool begin() override;
        void loop() override;
        bool executeCommand(DeviceCommand command, ArduinoMessage *message, ArduinoMessage *response) override;
        bool send(unsigned int protocol, unsigned int address, unsigned int command);
		IRSendData* getLastSend(){ return &ir2send; }
		void setStatusInfo(ArduinoMessage* message) override;
	};
} //end namespace	