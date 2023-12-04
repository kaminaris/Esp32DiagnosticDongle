#pragma once
#include <Arduino.h>

#include "NimBLECharacteristic.h"
#include "packets.h"

class UARTWrapper {
	public:
	bool started = false;
	NimBLECharacteristic* bleCharacteristic = nullptr;
	QueueDefinition* queue = xQueueCreate(10, sizeof(UartDataPacket));
	void begin(UartBeginRequest* request, NimBLECharacteristic* bleChar);
	void end();
	void send(UartDataPacket* pPacket);
	void onReceive() const;
	static void sendQueue(void* p);
};
