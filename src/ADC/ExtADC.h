#pragma once
#include <Arduino.h>

#include "ADS1X15.h"

constexpr uint8_t numberOfChannels = 3;

typedef std::function<void(uint16_t[numberOfChannels])> ExtADCCallback;

class ExtADC {
	public:
	uint8_t readyPin = 0;
	volatile bool conversionReady = false;
	volatile bool conversionStarted = false;
	uint8_t i2cAddress = 0;
	float maxVoltage = 0;
	ADS1115* ads = nullptr;
	uint16_t values[numberOfChannels] = {0};
	TaskHandle_t taskPointer;
	ExtADCCallback readyCallback = nullptr;

	explicit ExtADC(uint8_t address, uint8_t rdyPin);

	void setCallback(ExtADCCallback fn);
	void init();
	void startConversion();
	void stopConversion();
	void toggleConversion(bool toggle);
	static void handleConversion(void * instance);
	static void adcReady(void * instance);

	protected:
	uint8_t currentChannel = 0;
};
