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

static ReceiverStatus _stationBStatus[STATIONB_ADDRESS_COUNT];
static bool _lastSendSuccess = false;

// Opens the correct reading pipe for LINK A and returns to listening.
// This is the CubeSat's default/resting state.
static void _enterListen() {
    if (SOIL_TO_CUBE_MODE == RfDistributionMode::SHARED_MULTI_RECEIVER) {
        _radio.openReadingPipe(1, SHARED_ADDRESS_A_TO_CUBE);
    } else {
        _radio.openReadingPipe(1, MY_ADDRESS_FROM_A);
    }
    _radio.startListening();
}

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
    // Global ACK stays enabled; SHARED-mode writes use the per-packet
    // NO_ACK override instead of toggling this per link (see radio_config.h
    // header comment / design notes) - this is what lets Link A and Link B
    // run different modes on one physical radio without cross-talk.
    _radio.setAutoAck(true);

    for (uint8_t i = 0; i < STATIONB_ADDRESS_COUNT; i++) {
        _stationBStatus[i] = ReceiverStatus{false, 0, 0};
    }

    _enterListen();
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

bool radio_pollSoil(SoilPacket& outPacket) {
    if (!_radioAvailable) {
        return false;
    }
    if (!_radio.available()) {
        return false;
    }

    uint8_t buffer[RF_PAYLOAD_SIZE];
    _radio.read(buffer, RF_PAYLOAD_SIZE);

    if (buffer[0] != PACKET_SOIL) {
        // Not a Soil packet on this pipe - ignore safely, no error noise.
        return false;
    }

    memcpy(&outPacket, buffer, sizeof(SoilPacket));
    return true;
}

// Sends one already-padded 32-byte buffer per the CUBE_TO_STATIONB_MODE
// distribution rules. This is the single mechanism that implements all
// three RF distribution modes - see design notes.
static void _distributeSend(const uint8_t* buffer, SendOutcome& result) {
    if (CUBE_TO_STATIONB_MODE == RfDistributionMode::SHARED_MULTI_RECEIVER) {
        _radio.openWritingPipe(SHARED_ADDRESS_CUBE_TO_B);
        result.anyTransmitAttempted = true;
        bool ok = _radio.write(buffer, RF_PAYLOAD_SIZE, true); // NO_ACK
        result.anySuccess = result.anySuccess || ok;
        if (ok) result.successCount++; else result.failureCount++;
    } else {
        uint8_t destCount = (CUBE_TO_STATIONB_MODE == RfDistributionMode::ONE_TO_ONE)
                                 ? 1
                                 : STATIONB_ADDRESS_COUNT;
        for (uint8_t i = 0; i < destCount; i++) {
            _radio.openWritingPipe(STATIONB_ADDRESSES[i]);
            result.anyTransmitAttempted = true;
            bool ok = _radio.write(buffer, RF_PAYLOAD_SIZE);
            _stationBStatus[i].lastSuccess = ok;
            _stationBStatus[i].lastAttemptMs = millis();
            if (ok) {
                result.successCount++;
                result.anySuccess = true;
            } else {
                result.failureCount++;
                _stationBStatus[i].failureCount++;
            }
        }
    }
}

SendOutcome radio_relayAndSend(const SoilPacket* soilToRelay, const CubePacket& cubePacket) {
    SendOutcome result{};
    if (!_radioAvailable) {
        return result;
    }

    _radio.stopListening();

    if (soilToRelay != nullptr) {
        uint8_t soilBuf[RF_PAYLOAD_SIZE];
        memset(soilBuf, 0, RF_PAYLOAD_SIZE);
        memcpy(soilBuf, soilToRelay, sizeof(SoilPacket));
        _distributeSend(soilBuf, result);
    }

    uint8_t cubeBuf[RF_PAYLOAD_SIZE];
    memset(cubeBuf, 0, RF_PAYLOAD_SIZE);
    memcpy(cubeBuf, &cubePacket, sizeof(CubePacket));
    _distributeSend(cubeBuf, result);

    _enterListen(); // always return to the resting/listening state
    _lastSendSuccess = result.anySuccess;
    return result;
}

bool radio_getLastSendSuccess() {
    return _lastSendSuccess;
}

const ReceiverStatus* radio_getStationBStatus(uint8_t index) {
    if (CUBE_TO_STATIONB_MODE == RfDistributionMode::SHARED_MULTI_RECEIVER) {
        return nullptr;
    }
    if (index >= STATIONB_ADDRESS_COUNT) {
        return nullptr;
    }
    return &_stationBStatus[index];
}

uint8_t radio_getStationBCount() {
    if (CUBE_TO_STATIONB_MODE == RfDistributionMode::SHARED_MULTI_RECEIVER) {
        return 0;
    }
    return (CUBE_TO_STATIONB_MODE == RfDistributionMode::ONE_TO_ONE) ? 1 : STATIONB_ADDRESS_COUNT;
}
