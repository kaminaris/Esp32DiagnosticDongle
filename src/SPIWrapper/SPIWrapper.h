#pragma once
#include <Arduino.h>
#include <SPI.h>

#include "NimBLECharacteristic.h"
#include "packets.h"

class SPIWrapper {
	public:
	bool started = false;

	void begin();
	void end();
	void transaction(SPIDataPacket* pPacket);
};
