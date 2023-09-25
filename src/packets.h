#pragma once

#include <Arduino.h>

#define Packed __attribute__((packed))

enum class ResponseCode: uint8_t {
	OK = 7,
	FAIL,
	UNKNOWN_PACKET,
	PROGRESS,
	FILE,
	FILE_CONTENT,
};

enum PacketType: uint8_t {
	PING = 1,
	GET_CHIP_INFO,
	RESTART,
	FIRMWARE_UPDATE,

	PIN_CONFIGURE = 20,
	PIN_WRITE,
	PIN_READ,
	PIN_ANALOG_READ,

	CAN_BEGIN = 30,
	CAN_END,
	CAN_SEND,

	START_SAMPLING = 40,
	END_SAMPLING
};

struct Packed PinConfigPacket {
	uint8_t pin;
	uint8_t mode;
};

struct Packed PinWritePacket {
	uint8_t pin;
	uint8_t value;
};

struct Packed PinReadResult {
	uint8_t pin;
	int32_t value;
};

struct Packed CanBeginPacket {
	long baud;
};

struct Packed CanBeginResponse {
	int result;
};

struct Packed CanFramePacket {
	long id;
	uint8_t extended;
	uint8_t remoteTransmissionRequest;
	int requestLength;
	int packetSize;
	uint8_t data[64];
};

struct Packed SamplingPacket {
	uint16_t io12;
	uint16_t io13;
	uint16_t io14;

	uint16_t io26;
	uint16_t io27;
	uint16_t io32;

	uint16_t io33;
	uint16_t io34;
	uint16_t io35;
};

// System packets

struct Packed PingPacket {
	uint8_t q;
};

struct Packed BasicResponse {
	uint8_t r;
};

struct Packed ProgressResponse {
	uint8_t r;
	uint8_t progress;
};

struct Packed FirmwareUpdateRequest {
	uint32_t chunk;
	uint32_t chunks;
	uint16_t size;
	uint32_t totalSize;
	uint32_t checksum;
	uint8_t d[450];
};