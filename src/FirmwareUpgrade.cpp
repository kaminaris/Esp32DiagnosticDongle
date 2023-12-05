module;

#include <Arduino.h>
#include <Update.h>

export module FirmwareUpgrade;

export struct __attribute__((packed)) FirmwareUpdateRequest {
	uint32_t chunk;
	uint32_t chunks;
	uint16_t size;
	uint32_t totalSize;
	uint32_t checksum;
	uint8_t d[450];
};

export class FirmwareUpgrade {
	public:
	int progress = 0;

	int upgradeChunk(FirmwareUpdateRequest* request) {
		if (request->chunk == 1) {
			if (!Update.begin(request->totalSize, U_FLASH)) {
				Serial.println("Cannot start flash update!");
				return ESP_FAIL;
			}

			Update.write(request->d, request->size);
		}
		else if (request->chunk == request->chunks) {
			Update.write(request->d, request->size);
			if (Update.end()) {
				Serial.println("Update finished!");
				return ESP_OK;
			}
			else {
				Serial.printf("Update error: %d\n", Update.getError());
				return ESP_FAIL;
				// Serial::respondFail();
			}
		}
		else {
			Update.write(request->d, request->size);
		}

		progress = static_cast<uint8_t>((request->chunk / static_cast<double>(request->chunks)) * 100.0);

		if (request->chunk % 50 == 0) {
			Serial.printf("firmware update chunk: %d/%d progress: %d%%\n", request->chunk, request->chunks, progress);
		}

		return 0;
	}
};
