# Room 1 / Stage 1 — EVA's Last Beacon

## Story beat

At 03:17:42 UTC, EVA's final act is to broadcast a short emergency beacon. It is the first sign that some part of the global communication system is still alive.

## Participant task

Use the supplied ESP32 receiver to listen for the ESP-NOW transmission. Read the message in Serial Monitor at **115200 baud**.

## Expected result

The beacon identifies **Room 1**, gives the first recovered code `123456`, and directs the group to **Room 2**.

## Files and setup

- `Transmitter/Transmitter.ino` — upload to the front-desk ESP32. It broadcasts every two seconds.
- `Receiver/Receiver.ino` — upload to a separate ESP32 for the participant receiver.

No external wiring is needed; both boards use USB power/programming only.

## Facilitator note

Start the transmitter before the group enters. Do not tell participants the code; the receiver output is their first successful recovery.
