#pragma once

#include <Arduino.h>

#define Packed __attribute__((packed))

enum class ResponseCode: uint8_t {
	OK = 7,
	FAIL,
	UNKNOWN_PACKET,
	PROGRESS,

	SAMPLE_RESULT = 40,
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

	SAMPLE_CONTROL = 40,
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
	uint8_t data[8];
};

struct Packed SamplingControlPacket {
	uint8_t channel1;
	uint8_t channel2;
	uint8_t channel3;
};

struct Packed SamplingPacket {
	uint16_t rail5VChannel1;
	uint16_t rail5VChannel2;
	uint16_t rail5VChannel3;

	uint16_t rail20VChannel1;
	uint16_t rail20VChannel2;
	uint16_t rail20VChannel3;

	uint16_t rail200VChannel1;
	uint16_t rail200VChannel2;
	uint16_t rail200VChannel3;
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