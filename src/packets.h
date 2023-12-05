#pragma once

#include <Arduino.h>

#define Packed __attribute__((packed))

enum class ResponseCode : uint8_t {
	OK = 7,
	FAIL,
	UNKNOWN_PACKET,
	PROGRESS,
};

enum PacketType : uint8_t {
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

	UART_BEGIN = 50,
	UART_END,
	UART_SEND,

	I2C_BEGIN = 60,
	I2C_END,
	I2C_SEND,
	I2C_TRANSACTION,
	I2C_SCAN,

	SPI_BEGIN = 70,
	SPI_END,
	SPI_TRANSACTION,
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

struct Packed ResultResponse {
	ResponseCode code;
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

struct Packed SPIDataPacket {
	uint32_t clock;
	uint8_t bitOrder;
	uint8_t dataMode;
	uint32_t dataLength;
	uint8_t data[128];
	uint8_t result[128];
};

struct Packed I2cBeginRequest {
	uint32_t frequency;
};

struct Packed I2cDataPacket {
	uint8_t address;
	uint32_t dataLength;
	uint8_t data[256];
};

struct Packed I2cTransactionPacket {
	uint8_t address;

	uint32_t dataLength;
	uint8_t data[128];

	uint32_t requestLength;
	uint8_t result[128];
};

struct Packed I2CScanResult {
	uint8_t found;
	uint8_t addresses[128];
};

// System packets

struct Packed PingPacket {
	uint8_t q;
};

struct Packed ProgressResponse {
	uint8_t r;
	uint8_t progress;
};