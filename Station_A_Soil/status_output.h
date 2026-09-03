#pragma once
//
// status_output.h - periodic on-bench debug status line for Station A.
// This is separate from the RF protocol entirely - it exists purely to
// let you watch subsystem health over USB Serial while debugging.
//
// This is the ONLY file in this project allowed to touch Serial.
//
#include <stdint.h>
#include "packet_types.h"

void status_output_init();

// pkt: the latest SoilPacket - Dht/Light "Working" and their printed
//      values are derived from pkt.statusFlags and pkt's fields directly.
// rainWorking/soilWorking: the stuck-value debug heuristic (not part of
//      statusFlags - see rain_sensor.h / soil_moisture.h).
// radioAvailable: is the nRF24 chip itself initialized.
// lastSendSuccess: did the most recent radio_send() report anySuccess.
void status_output_print(const SoilPacket& pkt, bool rainWorking, bool soilWorking,
                          bool radioAvailable, bool lastSendSuccess);