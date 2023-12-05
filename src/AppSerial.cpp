module;

#include <Arduino.h>
#include <NimBLEAdvertising.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>

#include "ADC/ExtADC.h"
#include "CANWrapper/CANWrapper.h"
#include "I2CWrapper/I2CWrapper.h"
#include "SPIWrapper/SPIWrapper.h"
#include "packets.h"

export module AppSerial;

import UARTWrapper;
import FirmwareUpgrade;

constexpr char BLE_DEV_NAME[] = "DiagnosticDongle";
constexpr char SERVICE_UUID[] = "9C12D201-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_TX[] = "9C12D202-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_RX[] = "9C12D203-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_DEBUG[] = "9C12D204-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_CAN[] = "9C12D205-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_SAMPLING[] = "9C12D206-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_UART[] = "9C12D207-CBC3-413B-963B-9E49FF7E7D61";
constexpr char CHARACTERISTIC_UUID_I2C[] = "9C12D208-CBC3-413B-963B-9E49FF7E7D61";

ExtADC ADS5V(0x48, 14);
ExtADC ADS20V(0x49, 13);
ExtADC ADS200V(0x4A, 12);

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic;
NimBLECharacteristic* pDebugCharacteristic;
NimBLECharacteristic* pCanCharacteristic;
NimBLECharacteristic* pSamplingCharacteristic;
NimBLECharacteristic* pUartCharacteristic;
NimBLECharacteristic* pI2CCharacteristic;

ResultResponse okResponse = {.code = ResponseCode::OK, .result = 0};
ResultResponse failResponse = {.code = ResponseCode::FAIL, .result = 0};
ResultResponse unknownPacketResponse = {.code = ResponseCode::UNKNOWN_PACKET, .result = 0};

UARTWrapper* uartWrapper;
FirmwareUpgrade* upgrade;
CANWrapper canWrapper;
I2CWrapper i2cWrapper;
SPIWrapper spiWrapper;

// #define DUAL_SERIAL

class MyServerCallbacks : public NimBLEServerCallbacks {
	void onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) override {
		// deviceConnected = true;
		Serial.print("Client address: ");
		Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
		server->updateConnParams(desc->conn_handle, 6, 6, 0, 60);
	}

	void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) override {
		Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
	}

	void onDisconnect(NimBLEServer* server) override {
		// deviceConnected = false;
		NimBLEDevice::startAdvertising();
	}
};



export class AppSerial : public Print {
	public:
	class MyCallbacks : public NimBLECharacteristicCallbacks {
		public:
		void onWrite(NimBLECharacteristic* pCharacteristic) override {
			auto rxValue = pCharacteristic->getValue();

			if (rxValue.length() > 0) {
				auto* data = (uint8_t*)(rxValue.data() + 1);
				// Serial.println("*********");
				//  Serial.print("Received Value: ");
				//  for (int i = 0; i < rxValue.length(); i++) Serial.print(rxValue[i]);

				auto packetType = rxValue[0];
				// Serial.printf("Received packet: %d", packetType);

				switch (packetType) {
					case PIN_CONFIGURE: {
						auto* request = (PinConfigPacket*)data;

						pinMode(request->pin, request->mode);

						AppSerial::respondOk();
						break;
					}

					case PIN_WRITE: {
						auto* request = (PinWritePacket*)data;

						digitalWrite(request->pin, request->value);

						AppSerial::respondOk();
						break;
					}

					case PIN_READ: {
						auto* request = (PinWritePacket*)data;	// we can use same packet

						auto result = digitalRead(request->pin);

						struct PinReadResult readResponse = {.pin = request->pin, .value = result};

						pTxCharacteristic->setValue((uint8_t*)&readResponse, sizeof(readResponse));
						pTxCharacteristic->notify();
						break;
					}

					case PIN_ANALOG_READ: {
						auto* request = (PinWritePacket*)data;	// we can use same packet

						auto result = analogRead(request->pin);

						struct PinReadResult readResponse = {.pin = request->pin, .value = result};

						pTxCharacteristic->setValue((uint8_t*)&readResponse, sizeof(readResponse));
						pTxCharacteristic->notify();
						break;
					}

					case CAN_BEGIN: {
						auto* request = (CanBeginPacket*)data;	// we can use same packet
						auto result = canWrapper.begin(request, pCanCharacteristic);

						AppSerial::respondResult((result == ESP_OK) ? ResponseCode::OK : ResponseCode::FAIL, result);
						break;
					}

					case CAN_END: {
						canWrapper.end();

						AppSerial::respondOk();
						break;
					}

					case CAN_SEND: {
						auto* request = (CanFramePacket*)data;
						auto result = canWrapper.send(request);

						AppSerial::respondResult(result == 1 ? ResponseCode::OK : ResponseCode::FAIL, result);
						break;
					}

					case SAMPLE_CONTROL: {
						auto* request = (SamplingControlPacket*)data;

						ADS5V.toggleConversion(request->channel1);
						ADS20V.toggleConversion(request->channel2);
						ADS200V.toggleConversion(request->channel3);

						AppSerial::respondOk();
						break;
					}

					case UART_BEGIN: {
						// auto request = (UARTWrapper::UartBeginRequest*)data;
						uartWrapper->begin(data, pUartCharacteristic);

						AppSerial::respondOk();
						break;
					}

					case UART_END: {
						uartWrapper->end();

						AppSerial::respondOk();
						break;
					}

					case UART_SEND: {
						uartWrapper->send(data);

						AppSerial::respondOk();
						break;
					}

					case I2C_BEGIN: {
						auto request = (I2cBeginRequest*)data;
						auto result = i2cWrapper.begin(request, pI2CCharacteristic);

						AppSerial::respondResult(result == ESP_OK ? ResponseCode::OK : ResponseCode::FAIL, result);
						break;
					}

					case I2C_END: {
						i2cWrapper.end();
						AppSerial::respondOk();
						break;
					}

					case I2C_SEND: {
						auto request = (I2cDataPacket*)data;
						auto result = i2cWrapper.send(request);

						AppSerial::respondResult(result == ESP_OK ? ResponseCode::OK : ResponseCode::FAIL, result);
						break;
					}

					case I2C_TRANSACTION: {
						auto request = (I2cTransactionPacket*)data;
						auto result = i2cWrapper.transaction(request);
						if (result == ESP_OK) {
							pTxCharacteristic->setValue((uint8_t*)request, sizeof(I2cTransactionPacket));
							pTxCharacteristic->notify();
						}
						break;
					}

					case I2C_SCAN: {
						auto result = i2cWrapper.scan();

						pTxCharacteristic->setValue((uint8_t*)&result, sizeof(result));
						pTxCharacteristic->notify();

						break;
					}

					case SPI_BEGIN: {
						spiWrapper.begin();
						AppSerial::respondOk();
						break;
					}

					case SPI_END: {
						spiWrapper.end();
						AppSerial::respondOk();
						break;
					}

					case SPI_TRANSACTION: {
						auto request = (SPIDataPacket*)data;
						spiWrapper.transaction(request);

						pTxCharacteristic->setValue((uint8_t*)request, sizeof(SPIDataPacket));
						pTxCharacteristic->notify();
						break;
					}

						// System responses

					case PING: {
						auto* request = (PingPacket*)data;

						if (request->q == 128) {
							AppSerial::respondOk();
						}
						else {
							AppSerial::respondFail();
						}
						break;
					}

					case GET_CHIP_INFO: {
						uint64_t mac = ESP.getEfuseMac();

						String info = "";

						info += "\nChip model: " + String(ESP.getChipModel());
						info += "\nChip cores: " + String(ESP.getChipCores());
						info += "\nChip frequency: " + String(ESP.getCpuFreqMHz()) + "Mhz";
						info += "\nChip version: " + String(ESP.getChipRevision());

						info += "\nRAM size: " + String((ESP.getHeapSize() / 1024.0), 0) + "kB";
						info += "\nPSRAM size: " + String((ESP.getPsramSize() / (1024.0 * 1024.0)), 0) + "MB";

						info += "\nFlash size: " + String((ESP.getFlashChipSize() / (1024.0 * 1024.0)), 0) + "MB";
						info += "\nFlash speed: " + String((ESP.getFlashChipSpeed() / 1000000.0), 0) + "Mhz";

						info += "\nSDK version: " + String(ESP.getSdkVersion());
						info += "\nFirmware size: " + String((ESP.getSketchSize() / (1024.0 * 1024.0)), 0) + "MB";
						info += "\nMAC address: ";

						for (int i = 0; i < 6; i++) {
							if (i > 0) {
								info += "-";
							}
							info += String(byte(mac >> (i * 8) & 0xFF), HEX);
						}

						Serial.println(info);
						AppSerial::respondOk();
						break;
					}

					case FIRMWARE_UPDATE: {
						// auto* request = (FirmwareUpdateRequest*)data;
						// upgrade.upgradeChunk(request);

						// auto chunkCrc = CRC32.crc32(request->d, request->size);
						// if (chunkCrc != request->checksum) {
						// 	appSerial.printf(
						// 		"Chunk CRC does not match! Calculated: %08X Given: %08X \n", chunkCrc, request->checksum
						// 	);
						//
						// 	Update.rollBack();
						// 	Update.abort();
						// 	AppSerial::respondFail();
						// 	break;
						// }

						// if (request->chunk == 1) {
						// 	if (!Update.begin(request->totalSize, U_FLASH)) {
						// 		appSerial.println("Cannot start flash update!");
						// 		AppSerial::respondFail();
						// 		break;
						// 	}
						// 	Update.write(request->d, request->size);
						// }
						// else if (request->chunk == request->chunks) {
						// 	Update.write(request->d, request->size);
						// 	if (Update.end()) {
						// 		appSerial.println("Update finished!");
						// 		AppSerial::respondOk();
						// 	}
						// 	else {
						// 		appSerial.printf("Update error: %d\n", Update.getError());
						// 		AppSerial::respondFail();
						// 	}
						// }
						// else {
						// 	Update.write(request->d, request->size);
						// }
						//
						// if (request->chunk % 50 == 0) {
						// 	progressResponse.progress =
						// 		static_cast<uint8_t>((request->chunk / static_cast<double>(request->chunks)) * 100.0);
						//
						// 	appSerial.printf(
						// 		"firmware update chunk: %d/%d progress: %d%%\n",
						// 		request->chunk,
						// 		request->chunks,
						// 		progressResponse.progress
						// 	);
						//
						// 	pTxCharacteristic->setValue((uint8_t*)&progressResponse, sizeof(progressResponse));
						// 	pTxCharacteristic->notify();
						// }

						// do not respond
						// AppSerial::respondOk();
						break;
					}

					case RESTART: {
						// TestModule::prr(8);
						delay(1000);
						ESP.restart();
						break;
					}

					default: {
						AppSerial::respondUnknownPacket();
						break;
					}
				}
			}
		}
	};

	uint8_t sampleBuffer[7];
	void channelConversionReadyCh(uint8_t rail, uint16_t ch[numberOfChannels]) {
		sampleBuffer[0] = rail;
		memcpy(sampleBuffer + 1, ch, 6);
		// appSerial.notifySampling(sampleBuffer, 7);
	}

	void channelConversionReadyCh1(uint16_t ch[numberOfChannels]) {
		channelConversionReadyCh(0, ch);
	}

	void channelConversionReadyCh2(uint16_t ch[numberOfChannels]) {
		channelConversionReadyCh(1, ch);
	}

	void channelConversionReadyCh3(uint16_t ch[numberOfChannels]) {
		channelConversionReadyCh(2, ch);
	}

	static void notifyUart(uint8_t* data, size_t length) {
		pUartCharacteristic->setValue(data, length);
		pUartCharacteristic->notify();
	}

	void setup() {
		uartWrapper = new UARTWrapper(notifyUart);
		// Create the BLE Device
		NimBLEDevice::init(BLE_DEV_NAME);
		NimBLEDevice::setPower(ESP_PWR_LVL_P6);
		NimBLEDevice::setMTU(512);

		// Create the BLE Server
		pServer = NimBLEDevice::createServer();
		pServer->setCallbacks(new MyServerCallbacks());

		// Create the BLE Service
		auto pService = pServer->createService(SERVICE_UUID);

		// Create a BLE Characteristic
		pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);
		pDebugCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_DEBUG, NIMBLE_PROPERTY::NOTIFY);
		pCanCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_CAN, NIMBLE_PROPERTY::NOTIFY);
		pSamplingCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_SAMPLING, NIMBLE_PROPERTY::NOTIFY);
		pUartCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_UART, NIMBLE_PROPERTY::NOTIFY);
		pI2CCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_I2C, NIMBLE_PROPERTY::NOTIFY);

		auto pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE_NR);
		pRxCharacteristic->setCallbacks(new MyCallbacks());

		// Start the service
		pService->start();

		// Start the server
		pServer->start();

		NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
		pAdvertising->addServiceUUID(pService->getUUID());
		/** If your device is battery powered you may consider setting scan response
		 *  to false as it will extend battery life at the expense of less data sent.
		 */
		// pAdvertising->setScanResponse(true);
		// Start advertising
		pAdvertising->start();

		Serial.println("Waiting a client connection to notify...");

		Wire1.begin(33, 32);
		ADS5V.init();
		ADS20V.init();
		ADS200V.init();

		// ADS5V.startConversion();
		// ADS20V.startConversion();
		// ADS200V.startConversion();
		using namespace std::placeholders;

		ADS5V.setCallback(std::bind(&AppSerial::channelConversionReadyCh1, this, _1));
		ADS20V.setCallback(std::bind(&AppSerial::channelConversionReadyCh2, this, _1));
		ADS200V.setCallback(std::bind(&AppSerial::channelConversionReadyCh3, this, _1));
	}

	static void respondOk() {
		pTxCharacteristic->setValue((uint8_t*)&okResponse, sizeof(okResponse));
		pTxCharacteristic->notify();
	}

	static void respondFail() {
		pTxCharacteristic->setValue((uint8_t*)&failResponse, sizeof(failResponse));
		pTxCharacteristic->notify();
	}

	static void respondResult(ResponseCode code, int result) {
		struct ResultResponse response = {.result = result};

		pTxCharacteristic->setValue((uint8_t*)&response, sizeof(response));
		pTxCharacteristic->notify();
	}

	static void respondUnknownPacket() {
		pTxCharacteristic->setValue((uint8_t*)&unknownPacketResponse, sizeof(unknownPacketResponse));
		pTxCharacteristic->notify();
	}

	size_t write(uint8_t character) override {
#ifdef DUAL_SERIAL
		Serial.write(character);
#endif
		if (pDebugCharacteristic == nullptr) {
			return 0;
		}

		pDebugCharacteristic->setValue(&character, 1);
		pDebugCharacteristic->notify();

		return 1;
	}

	size_t write(const uint8_t* buffer, size_t size) override {
#ifdef DUAL_SERIAL
		Serial.write(buffer, size);
#endif
		if (pDebugCharacteristic == nullptr) {
			return 0;
		}

		pDebugCharacteristic->setValue(buffer, size);
		pDebugCharacteristic->notify();

		return size;
	}

	void notifySampling(const uint8_t* buffer, size_t size) {
		pSamplingCharacteristic->setValue(buffer, size);
		pSamplingCharacteristic->notify();
	}
};