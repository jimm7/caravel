/* SPDX-License-Identifier: GPL-3.0-only
 * Caravel Plus experimental BLE-MIDI input adapter.
 *
 * HOST TEST ONLY until radio-driver integration is complete. This source is
 * excluded by default and build.py refuses to create a radio-less .fwsc
 * with CARAVEL_BLE_MIDI_BRIDGE=1.
 *
 * Context and ownership:
 * - one future radio ISR producer calls ble_bridge_radio_receive()
 * - TIMER5 at 2 kHz calls ble_bridge_timer5_poll(), the only existing MIDI
 *   queue producer together with USB/TRS, via midi_enqueue()
 * - the audio ISR consumes midi_in_q as before
 *
 * Not a Bluetooth stack: no radio power-up, GATT, OTA, calibration, pairing,
 * or transmission code lives here. Only channel voice and transport realtime
 * messages are admitted; SysEx, vendor commands and malformed packets drop.
 */
#ifndef CARAVEL_BLE_MIDI_BRIDGE
#error "CARAVEL_BLE_MIDI_BRIDGE must be defined explicitly for the host test"
#endif
#if !CARAVEL_BLE_MIDI_BRIDGE
#error "Do not include the BLE receive adapter when disabled"
#endif

#define BLE_RX_Q 32u
#define BLE_RX_DRAIN 8u
static uint32_t ble_rx_pkt[BLE_RX_Q];
static volatile uint32_t ble_rx_write, ble_rx_read;
static volatile uint8_t ble_rx_overrun, ble_rx_link_lost;

/* Input is a USB-MIDI-format 32-bit event packed by the future BLE stack. */
static int ble_bridge_safe(uint32_t pkt)
{
    uint32_t cin = pkt & 15u, status = (pkt >> 8) & 255u;
    uint32_t data1 = (pkt >> 16) & 255u, data2 = (pkt >> 24) & 255u;
    if (cin == 15u)
        return status == 0xF8u || status == 0xFAu ||
               status == 0xFBu || status == 0xFCu;
    if (cin < 8u || cin > 14u || status < 0x80u || status >= 0xF0u)
        return 0;
    if ((status >> 4) != cin || data1 >= 0x80u)
        return 0;
    if (cin != 0xCu && cin != 0xDu && data2 >= 0x80u)
        return 0;
    return 1;
}

/* Only the dedicated radio IRQ producer will call this once the port exists. */
static int ble_bridge_radio_receive(uint32_t pkt)
{
    uint32_t w;
    if (ble_rx_link_lost || !ble_bridge_safe(pkt))
        return 0;                                /* drop SysEx/invalid safely */
    w = ble_rx_write;
    if (w - ble_rx_read >= BLE_RX_Q) {
        ble_rx_overrun = 1u;
        return 0;
    }
    ble_rx_pkt[w % BLE_RX_Q] = pkt;
    RING_PUBLISH();
    ble_rx_write = w + 1u;
    return 1;
}

/* TIMER5 context: the only caller of midi_enqueue for this bridge. */
/* The radio IRQ reports link loss without mutating Caravel's MIDI/synth state.
 * TIMER5 requests the existing held-note recovery after dropping stale input. */
static void ble_bridge_radio_link_lost(void)
{
    ble_rx_link_lost = 1u;
    RING_PUBLISH();
}

static void ble_bridge_timer5_poll(void)
{
    uint32_t n = 0;
    if (ble_rx_overrun || ble_rx_link_lost) {
        /* A lost note-off means the synth MUST release held notes. */
        ble_rx_read = ble_rx_write;
        ble_rx_overrun = ble_rx_link_lost = 0u;
        midi_in_overflow = 1u;
        return;
    }
    while (ble_rx_read != ble_rx_write && n++ < BLE_RX_DRAIN) {
        uint32_t pkt = ble_rx_pkt[ble_rx_read % BLE_RX_Q];
        ble_rx_read++;
        if (!midi_enqueue(pkt, 3u)) {
            /* Existing audio-side overflow recovery handles panic/reset. */
            ble_rx_read = ble_rx_write;
            midi_in_overflow = 1u;
            return;
        }
    }
}

/* Useful only for host tests / future radio power-cycle handling. */
static void ble_bridge_reset(void)
{
    ble_rx_read = ble_rx_write = 0u;
    ble_rx_overrun = ble_rx_link_lost = 0u;
}
