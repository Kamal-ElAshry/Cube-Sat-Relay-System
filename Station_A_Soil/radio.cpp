#include "radio.h"
#include "radio_config.h"
#include "packet_types.h"
#include "config.h"

#include <RF24.h>
#include <SPI.h>
#include <string.h>

static RF24 _radio(PIN_RADIO_CE, PIN_RADIO_CSN);
static bool _radioAvailable = false;

constexpr unsigned long RADIO_RETRY_INTERVAL_MS = 5000UL;
static unsigned long _lastRetryMs = 0;

static ReceiverStatus _destStatus[CUBESAT_ADDRESS_COUNT];
static bool _lastSendSuccess = false;

bool radio_init() {
    _radioAvailable = _radio.begin();
    if (!_radioAvailable) {
        return false;
    }

    _radio.setChannel(RF_CHANNEL);
    _radio.setDataRate(RF_DATA_RATE);
    _radio.setPALevel(RF_PA_LEVEL);
    _radio.setAddressWidth(RF_ADDRESS_WIDTH);
    _radio.setRetries(RF_RETRY_DELAY, RF_RETRY_COUNT);
    _radio.setPayloadSize(RF_PAYLOAD_SIZE);
    _radio.setAutoAck(true); // per-packet NO_ACK override is used for SHARED mode writes instead
    _radio.stopListening();  // Station A never receives

    for (uint8_t i = 0; i < CUBESAT_ADDRESS_COUNT; i++) {
        _destStatus[i] = ReceiverStatus{false, 0, 0};
    }

    return true;
}

bool radio_isAvailable() {
    return _radioAvailable;
}

void radio_maintain() {
    if (_radioAvailable) {
        return;
    }
    unsigned long now = millis();
    if (now - _lastRetryMs >= RADIO_RETRY_INTERVAL_MS) {
        _lastRetryMs = now;
        radio_init();
    }
}

SendOutcome radio_send(const uint8_t* payload, uint8_t len) {
    SendOutcome result{};

    if (!_radioAvailable || len > RF_PAYLOAD_SIZE) {
        return result;
    }

    uint8_t buffer[RF_PAYLOAD_SIZE];
    memset(buffer, 0, RF_PAYLOAD_SIZE);
    memcpy(buffer, payload, len);

    if (SOIL_TO_CUBE_MODE == RfDistributionMode::SHARED_MULTI_RECEIVER) {
        _radio.openWritingPipe(SHARED_ADDRESS_A_TO_CUBE);
        result.anyTransmitAttempted = true;
        // multicast=true -> NO_ACK packet flag; no receiver confirms delivery.
        bool ok = _radio.write(buffer, RF_PAYLOAD_SIZE, true);
        result.anySuccess = ok;
        if (ok) result.successCount = 1; else result.failureCount = 1;
    } else {
        uint8_t destCount = (SOIL_TO_CUBE_MODE == RfDistributionMode::ONE_TO_ONE)
                                 ? 1
                                 : CUBESAT_ADDRESS_COUNT;
        for (uint8_t i = 0; i < destCount; i++) {
            _radio.openWritingPipe(CUBESAT_ADDRESSES[i]);
            result.anyTransmitAttempted = true;
            bool ok = _radio.write(buffer, RF_PAYLOAD_SIZE); // ACK required, no retry beyond hardware retries
            _destStatus[i].lastSuccess = ok;
            _destStatus[i].lastAttemptMs = millis();
            if (ok) {
                result.successCount++;
                result.anySuccess = true;
            } else {
                result.failureCount++;
                _destStatus[i].failureCount++;
            }
        }
    }

    _lastSendSuccess = result.anySuccess;
    return result;
}

bool radio_getLastSendSuccess() {
    return _lastSendSuccess;
}

const ReceiverStatus* radio_getDestinationStatus(uint8_t index) {
    if (SOIL_TO_CUBE_MODE == RfDistributionMode::SHARED_MULTI_RECEIVER) {
        return nullptr;
    }
    if (index >= CUBESAT_ADDRESS_COUNT) {
        return nullptr;
    }
    return &_destStatus[index];
}

uint8_t radio_getDestinationCount() {
    if (SOIL_TO_CUBE_MODE == RfDistributionMode::SHARED_MULTI_RECEIVER) {
        return 0;
    }
    return (SOIL_TO_CUBE_MODE == RfDistributionMode::ONE_TO_ONE) ? 1 : CUBESAT_ADDRESS_COUNT;
}
