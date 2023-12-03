#pragma once

#include <Arduino.h>
#include <CAN.h>

#include "NimBLECharacteristic.h"
#include "packets.h"

class CANWrapper {
	public:
	bool canStarted = false;
	QueueDefinition* canQueue = xQueueCreate(10, sizeof(CanFramePacket));
	NimBLECharacteristic* bleCharacteristic = nullptr;

	int begin(CanBeginPacket* request, NimBLECharacteristic* bleChar);
	void onReceive(int packetSize) const;
	static void sendQueue(void* p);
	void end();
	int send(CanFramePacket* packet);
};
