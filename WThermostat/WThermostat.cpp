#include <Arduino.h>
#include "../../WAdapter/Wadapter/WNetwork.h"
#include "WBecaDevice.h"
#include "WClock.h"

#define APPLICATION "Thermostat Beca"
#define VERSION "0.11"
#define DEBUG false


WNetwork* network;
WBecaDevice* becaDevice;
WClock* wClock;

void setup() {
	Serial.begin(9600);
	//Wifi and Mqtt connection
	network = new WNetwork(DEBUG, APPLICATION, VERSION, true, NO_LED);
	network->setOnNotify([]() {
		if (network->isWifiConnected()) {

		}
		if (network->isMqttConnected()) {
			becaDevice->queryState();
			if (becaDevice->isDeviceStateComplete()) {
				//sendMqttStatus();
			}
		}
	});
	network->setOnConfigurationFinished([]() {
		//Switch blinking thermostat in normal operating mode back
		becaDevice->cancelConfiguration();
	});
	//KaClock - time sync
	wClock = new WClock(network, APPLICATION);
	network->addDevice(wClock);
	wClock->setOnTimeUpdate([]() {
		becaDevice->sendActualTimeToBeca();
	});
	wClock->setOnError([](String error) {
		return network->publishMqtt("error", "message", error);
	});
	//Communication between ESP and Beca-Mcu
	becaDevice = new WBecaDevice(network, DEBUG, APPLICATION, network->getSettings(), wClock);
	network->addDevice(becaDevice);

	becaDevice->setOnSchedulesChange([]() {
		//Send schedules once at ESP start and at every change
		return true;// sendSchedulesViaMqtt();
	});
	becaDevice->setOnNotifyCommand([](String commandType) {
		return network->publishMqtt("mcucommand", commandType, becaDevice->getCommandAsString());
	});
	becaDevice->setOnConfigurationRequest([]() {
		network->startWebServer();
		return true;
	});
}

void loop() {
	network->loop(millis());
	delay(50);
}

/*bool sendClockStateViaMqttStatus() {
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	kClock->getMqttState(json, true);
	return network->publishMqtt("clock", json);
}*/

/**
 * Sends the schedule in 3 messages because of maximum message length
 */
/*
bool sendSchedulesViaMqtt() {
	boolean result = true;
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	becaDevice->getMqttSchedules(json, SCHEDULE_WORKDAY);
	result = result && network->publishMqtt("schedules", json);
	jsonBuffer.clear();
	JsonObject& json2 = jsonBuffer.createObject();
	becaDevice->getMqttSchedules(json2, SCHEDULE_SATURDAY);
	result = result && network->publishMqtt("schedules", json2);
	jsonBuffer.clear();
	JsonObject& json3 = jsonBuffer.createObject();
	becaDevice->getMqttSchedules(json3, SCHEDULE_SUNDAY);
	result = result && network->publishMqtt("schedules", json3);
	jsonBuffer.clear();
	return result;
}*/