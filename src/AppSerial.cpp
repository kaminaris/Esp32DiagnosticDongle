#include "AppSerial.h"

#include "I2CWrapper/I2CWrapper.h"

extern ExtADC ADS5V;
extern ExtADC ADS20V;
extern ExtADC ADS200V;

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic;
NimBLECharacteristic* pDebugCharacteristic;
NimBLECharacteristic* pCanCharacteristic;
NimBLECharacteristic* pSamplingCharacteristic;
NimBLECharacteristic* pUartCharacteristic;
NimBLECharacteristic* pI2CCharacteristic;
NimBLECharacteristic* pSPICharacteristic;

struct ResultResponse okResponse = {.code = ResponseCode::OK, .result = 0};
struct ResultResponse failResponse = {.code = ResponseCode::FAIL, .result = 0};
struct ResultResponse unknownPacketResponse = {.code = ResponseCode::UNKNOWN_PACKET, .result = 0};
struct ProgressResponse progressResponse = {.r = (uint8_t)ResponseCode::PROGRESS, .progress = 0};

UARTWrapper uartWrapper;
CANWrapper canWrapper;
I2CWrapper i2cWrapper;

void MyCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
	auto rxValue = pCharacteristic->getValue();

	if (rxValue.length() > 0) {
		auto* data = (uint8_t*)(rxValue.data() + 1);
		// Serial.println("*********");
		//  Serial.print("Received Value: ");
		//  for (int i = 0; i < rxValue.length(); i++) Serial.print(rxValue[i]);

		auto packetType = rxValue[0];
		// Serial.printf("Received packet: %d", packetType);

		switch (packetType) {
			case PacketType::PIN_CONFIGURE: {
				auto* request = (PinConfigPacket*)data;

				pinMode(request->pin, request->mode);

				AppSerial::respondOk();
				break;
			}

			case PacketType::PIN_WRITE: {
				auto* request = (PinWritePacket*)data;

				digitalWrite(request->pin, request->value);

				AppSerial::respondOk();
				break;
			}

			case PacketType::PIN_READ: {
				auto* request = (PinWritePacket*)data;	// we can use same packet

				auto result = digitalRead(request->pin);

				struct PinReadResult readResponse = {.pin = request->pin, .value = result};

				pTxCharacteristic->setValue((uint8_t*)&readResponse, sizeof(readResponse));
				pTxCharacteristic->notify();
				break;
			}

			case PacketType::PIN_ANALOG_READ: {
				auto* request = (PinWritePacket*)data;	// we can use same packet

				auto result = analogRead(request->pin);

				struct PinReadResult readResponse = {.pin = request->pin, .value = result};

				pTxCharacteristic->setValue((uint8_t*)&readResponse, sizeof(readResponse));
				pTxCharacteristic->notify();
				break;
			}

			case PacketType::CAN_BEGIN: {
				auto* request = (CanBeginPacket*)data;	// we can use same packet
				auto result = canWrapper.begin(request, pCanCharacteristic);

				AppSerial::respondResult((result == ESP_OK) ? ResponseCode::OK : ResponseCode::FAIL, result);
				break;
			}

			case PacketType::CAN_END: {
				canWrapper.end();

				AppSerial::respondOk();
				break;
			}

			case PacketType::CAN_SEND: {
				auto* request = (CanFramePacket*)data;
				auto result = canWrapper.send(request);

				AppSerial::respondResult(result == 1 ? ResponseCode::OK : ResponseCode::FAIL, result);
				break;
			}

			case PacketType::SAMPLE_CONTROL: {
				auto* request = (SamplingControlPacket*)data;

				ADS5V.toggleConversion(request->channel1);
				ADS20V.toggleConversion(request->channel2);
				ADS200V.toggleConversion(request->channel3);

				AppSerial::respondOk();
				break;
			}

			case PacketType::UART_BEGIN: {
				auto request = (UartBeginRequest*)data;
				uartWrapper.begin(request, pUartCharacteristic);

				AppSerial::respondOk();
				break;
			}

			case PacketType::UART_END: {
				uartWrapper.end();

				AppSerial::respondOk();
				break;
			}

			case PacketType::UART_SEND: {
				auto request = (UartDataPacket*)data;
				uartWrapper.send(request);

				AppSerial::respondOk();
				break;
			}

			case PacketType::I2C_BEGIN: {
				auto request = (I2cBeginRequest*)data;
				auto result = i2cWrapper.begin(request, pI2CCharacteristic);

				AppSerial::respondResult(result == ESP_OK ? ResponseCode::OK : ResponseCode::FAIL, result);
				break;
			}

			case PacketType::I2C_END: {
				i2cWrapper.end();
				AppSerial::respondOk();
				break;
			}

			case PacketType::I2C_SEND: {
				auto request = (I2cDataPacket*)data;
				auto result = i2cWrapper.send(request);

				AppSerial::respondResult(result == ESP_OK ? ResponseCode::OK : ResponseCode::FAIL, result);
				break;
			}

			case PacketType::I2C_TRANSACTION: {
				auto request = (I2cTransactionPacket*)data;
				auto result = i2cWrapper.transaction(request);
				if (result == ESP_OK) {
					pTxCharacteristic->setValue((uint8_t*)request, sizeof(I2cTransactionPacket));
					pTxCharacteristic->notify();
				}
				break;
			}

			case PacketType::I2C_SCAN: {
				auto result = i2cWrapper.scan();

				pTxCharacteristic->setValue((uint8_t*)&result, sizeof(result));
				pTxCharacteristic->notify();

				break;
			}

				// System responses

			case PacketType::PING: {
				auto* request = (PingPacket*)data;

				if (request->q == 128) {
					AppSerial::respondOk();
				}
				else {
					AppSerial::respondFail();
				}
				break;
			}

			case PacketType::GET_CHIP_INFO: {
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

				appSerial.println(info);
				AppSerial::respondOk();
				break;
			}

			case PacketType::FIRMWARE_UPDATE: {
				auto* request = (FirmwareUpdateRequest*)data;

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

				if (request->chunk == 1) {
					if (!Update.begin(request->totalSize, U_FLASH)) {
						appSerial.println("Cannot start flash update!");
						AppSerial::respondFail();
						break;
					}
					Update.write(request->d, request->size);
				}
				else if (request->chunk == request->chunks) {
					Update.write(request->d, request->size);
					if (Update.end()) {
						appSerial.println("Update finished!");
						AppSerial::respondOk();
					}
					else {
						appSerial.printf("Update error: %d\n", Update.getError());
						AppSerial::respondFail();
					}
				}
				else {
					Update.write(request->d, request->size);
				}

				if (request->chunk % 50 == 0) {
					progressResponse.progress =
						static_cast<uint8_t>((request->chunk / static_cast<double>(request->chunks)) * 100.0);

					appSerial.printf(
						"firmware update chunk: %d/%d progress: %d%%\n",
						request->chunk,
						request->chunks,
						progressResponse.progress
					);

					pTxCharacteristic->setValue((uint8_t*)&progressResponse, sizeof(progressResponse));
					pTxCharacteristic->notify();
				}

				// do not respond
				// AppSerial::respondOk();
				break;
			}

			case PacketType::RESTART: {
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

void MyServerCallbacks::onConnect(NimBLEServer* server, ble_gap_conn_desc* desc) {
	// deviceConnected = true;
	appSerial.print("Client address: ");
	appSerial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
	server->updateConnParams(desc->conn_handle, 6, 6, 0, 60);
}

void MyServerCallbacks::onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
	appSerial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
}

void MyServerCallbacks::onDisconnect(NimBLEServer* server) {
	// deviceConnected = false;
	NimBLEDevice::startAdvertising();
}

void AppSerial::setup() {
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
	pSPICharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_SPI, NIMBLE_PROPERTY::NOTIFY);

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

	appSerial.println("Waiting a client connection to notify...");
}

void AppSerial::respondOk() {
	pTxCharacteristic->setValue((uint8_t*)&okResponse, sizeof(okResponse));
	pTxCharacteristic->notify();
}

void AppSerial::respondFail() {
	pTxCharacteristic->setValue((uint8_t*)&failResponse, sizeof(failResponse));
	pTxCharacteristic->notify();
}

void AppSerial::respondResult(ResponseCode code, int result) {
	struct ResultResponse response = {.result = result};

	pTxCharacteristic->setValue((uint8_t*)&response, sizeof(response));
	pTxCharacteristic->notify();
}

void AppSerial::respondUnknownPacket() {
	pTxCharacteristic->setValue((uint8_t*)&unknownPacketResponse, sizeof(unknownPacketResponse));
	pTxCharacteristic->notify();
}

size_t AppSerial::write(uint8_t character) {
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

size_t AppSerial::write(const uint8_t* buffer, size_t size) {
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

void AppSerial::notifySampling(const uint8_t* buffer, size_t size) {
	pSamplingCharacteristic->setValue(buffer, size);
	pSamplingCharacteristic->notify();
}

AppSerial appSerial;