#pragma once
//
// status_output.h - periodic on-bench debug status line for the CubeSat.
// Separate from the RF protocol entirely - purely for watching subsystem
// health over USB Serial while debugging.
//
// This is the ONLY file in this project allowed to touch Serial.
//
#include <stdint.h>
#include "packet_types.h"

void status_output_init();

// pkt: the latest CubePacket - Mpu/Bmp "Working" and their printed
//      values are derived from pkt.statusFlags and pkt's fields directly.
// radioAvailable: is the nRF24 chip itself initialized (one physical
//      radio shared by both links).
// linkAReceived: has a valid Soil packet ever been received (Link A).
// linkBSent: did the most recent Link B transmit attempt report anySuccess.
void status_output_print(const CubePacket& pkt, bool radioAvailable,
                          bool linkAReceived, bool linkBSent);