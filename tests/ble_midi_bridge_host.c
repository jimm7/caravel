/* SPDX-License-Identifier: GPL-3.0-only
 * Unit-test the BLE-to-TIMER5 MIDI adapter without any RF hardware or SDK. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CARAVEL_BLE_MIDI_BRIDGE 1
#define RING_PUBLISH() __sync_synchronize()

static volatile uint8_t midi_in_overflow;
static uint32_t count, last_packet, last_source, should_fail;
static int midi_enqueue(uint32_t packet, uint32_t source)
{
    if (should_fail) return 0;
    count++;
    last_packet = packet;
    last_source = source;
    return 1;
}
#include "../firmware/src/ble_midi_bridge.c"

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAILED at line %d: %s\n", __LINE__, #c); exit(1); } } while (0)

static uint32_t packet(uint32_t status, uint32_t a, uint32_t b)
{
    return (status >> 4) | (status << 8) | (a << 16) | (b << 24);
}

int main(void)
{
    const uint32_t on = packet(0x90u, 60u, 100u);
    const uint32_t off = packet(0x80u, 60u, 0u);
    const uint32_t clock = 0xFu | (0xF8u << 8);
    uint32_t i;
    ble_bridge_reset();
    CHECK(ble_bridge_radio_receive(on));
    CHECK(ble_bridge_radio_receive(off));
    CHECK(ble_bridge_radio_receive(clock));
    CHECK(!ble_bridge_radio_receive(0x04u | (0xF0u << 8))); /* SysEx blocked */
    CHECK(!ble_bridge_radio_receive(packet(0x90u, 128u, 7u)));
    ble_bridge_timer5_poll();
    CHECK(count == 3u && last_packet == clock && last_source == 3u);
    CHECK(!midi_in_overflow);
    ble_bridge_reset();
    count = 0u;
    /* Saturation triggers a panic in the existing audio-side overflow path. */
    for (i = 0; i < BLE_RX_Q; i++) CHECK(ble_bridge_radio_receive(on));
    CHECK(!ble_bridge_radio_receive(off));
    ble_bridge_timer5_poll();
    CHECK(midi_in_overflow);
    CHECK(count == 0u); /* Never consume a partial note stream. */
    midi_in_overflow = 0;
    ble_bridge_reset();
    should_fail = 1u;
    CHECK(ble_bridge_radio_receive(on));
    ble_bridge_timer5_poll();
    CHECK(midi_in_overflow);
    should_fail = 0u;
    ble_bridge_reset();
    midi_in_overflow = 0;
    CHECK(!ble_bridge_radio_receive(packet(0xF0u, 1u, 2u)));
    CHECK(!ble_bridge_radio_receive(packet(0x90u, 60u, 200u)));
    CHECK(!midi_in_overflow);
    puts("BLE-MIDI bridge host tests passed (without radio hardware)");
    return 0;
}
