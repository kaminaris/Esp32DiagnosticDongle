#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "NimBLECharacteristic.h"
#include "packets.h"

typedef void (*WireCallback)(int);

class I2CWrapper {
	public:
	bool i2cStarted = false;
	QueueDefinition* i2cQueue = xQueueCreate(10, sizeof(I2cDataPacket));
	NimBLECharacteristic* bleCharacteristic = nullptr;

	int begin(I2cBeginRequest* request, NimBLECharacteristic* bleChar);
	void onReceive(int bytes);
	static void sendQueue(void* p);
	void end();
	uint8_t send(I2cDataPacket* request);
	int transaction(I2cTransactionPacket* request);
	I2CScanResult scan();
};
