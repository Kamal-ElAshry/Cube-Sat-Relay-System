#include "radio.h"
#include "radio_config.h"
#include "packet_types.h"
#include "config.h"

#include <RF24.h>
#include <SPI.h>

static RF24 _radio(PIN_RADIO_CE, PIN_RADIO_CSN);
static bool _radioAvailable = false;

constexpr unsigned long RADIO_RETRY_INTERVAL_MS = 5000UL;
static unsigned long _lastRetryMs = 0;

bool radio_init() {
    _radioAvailable = _radio.begin();
    if (!_radioAvailable) {
        return false;
    }

    _radio.setChannel(RF_CHANNEL);
    _radio.setDataRate(RF_DATA_RATE);
    _radio.setPALevel(RF_PA_LEVEL);
    _radio.setAddressWidth(RF_ADDRESS_WIDTH);
    _radio.setPayloadSize(RF_PAYLOAD_SIZE);
    _radio.setAutoAck(true); // NO_ACK packets from a SHARED-mode sender are never ACKed regardless

    if (CUBE_TO_STATIONB_MODE == RfDistributionMode::SHARED_MULTI_RECEIVER) {
        _radio.openReadingPipe(1, SHARED_ADDRESS_CUBE_TO_B);
    } else {
        _radio.openReadingPipe(1, MY_ADDRESS_FROM_CUBE);
    }

    _radio.startListening();
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

bool radio_pollPacket(uint8_t* outBuffer, uint8_t& outLen) {
    if (!_radioAvailable) {
        return false;
    }
    if (!_radio.available()) {
        return false;
    }

    _radio.read(outBuffer, RF_PAYLOAD_SIZE);
    outLen = RF_PAYLOAD_SIZE;
    return true;
}
