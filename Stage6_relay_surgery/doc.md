# Room 6 / Stage 6 — EVA's Hidden Synchronization Cache

## Story beat

The Room 5 conversation leads to a location outside the seven-node treaty. This is an emergency cache EVA created secretly. Its smartboard briefing explains that it trusts only a two-way ESP-NOW recovery request.

## Participant task

1. The facilitator uploads `relay/relay.ino` to the Room 6 ESP32 and notes the MAC address printed at 115200 baud.
2. Set that address in `NODE_6_MAC` inside `participant_terminal/participant_terminal.ino`, then upload it to the participant ESP32.
3. In the participant terminal's Serial Monitor, enter `123456` and send it.
4. The terminal sends protocol `EVA_RESTORE`; Node 6 replies through ESP-NOW.

Both boards use ESP-NOW channel 1 and must not join an access point.

## Expected result

The response restores EVA's hidden cache and reveals **Room 7** plus the last recorded key, `123456`.

## Files and hardware

- `briefing.html` — auto-scrolling Room 6 smartboard briefing.
- `relay/relay.ino` — Node 6 responder firmware.
- `participant_terminal/participant_terminal.ino` — participant request terminal.
- Hardware: two ESP32 boards and USB Serial Monitor access.

## Facilitator note

Prepare the Node 6 MAC address before the event, or clearly label the MAC-copy step. The participant terminal refuses to run while its placeholder MAC address is unchanged.
