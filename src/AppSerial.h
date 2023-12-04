#pragma once

#include <Arduino.h>
#include <CAN.h>
#include <NimBLEAdvertising.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <Update.h>

#include <cstring>

#include "ADC/ExtADC.h"
#include "CANWrapper/CANWrapper.h"
#include "I2CWrapper/I2CWrapper.h"
#include "SPIFFS.h"
#include "SPIWrapper/SPIWrapper.h"
#include "UARTWrapper/UARTWrapper.h"
#include "packets.h"

constexpr char BLE_DEV_NAME[] = "DiagnosticDongle";
constexpr char SERVICE_UUID[] = "9C12D201-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_TX[] = "9C12D202-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_RX[] = "9C12D203-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_DEBUG[] = "9C12D204-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_CAN[] = "9C12D205-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_SAMPLING[] = "9C12D206-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_UART[] = "9C12D207-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_I2C[] = "9C12D208-CBC3-413B-963B-9E49FF7E7D61";

#define DUAL_SERIAL

class MyServerCallbacks : public NimBLEServerCallbacks {
	void onDisconnect(NimBLEServer* pServer) override;

	void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;

	void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) override;
};

class MyCallbacks : public NimBLECharacteristicCallbacks {
	void onWrite(NimBLECharacteristic* pCharacteristic) override;
};

class AppSerial : public Print {
	public:
	size_t write(uint8_t) override;
	size_t write(const uint8_t* buffer, size_t size) override;
	void notifySampling(const uint8_t* buffer, size_t size);
	static void setup();
	static void respondOk();
	static void respondFail();
	static void respondResult(ResponseCode code, int result = 0);
	static void respondUnknownPacket();
};

extern AppSerial appSerial;