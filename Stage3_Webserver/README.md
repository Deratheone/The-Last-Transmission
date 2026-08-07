# Room 3 / Stage 3 — Companion-07 Memory Relay

## Story beat

Companion-07 is a small archived process that knew EVA before the world stopped listening. Its memory is fragmented, but it believes replaying a simple signal sequence can recover one useful trace.

## Participant task

1. Join the ESP32 access point `EVA-MEMORY-3` with password `TANCEI00`.
2. Open the IP address printed in Serial Monitor.
3. Complete three memory-pattern rounds by repeating the four-pad sequence in order.

## Expected result

Companion-07 remembers that EVA trusted people who could still communicate. The page reveals **Room 4** and `TIMEIS`.

## Files and hardware

- `webgame/webgame.ino` — ESP32 access point and embedded game; upload this to the Room 3 board.
- `webserver.html` — standalone browser preview of the same game.
- Hardware: one ESP32 and a phone/laptop connected to its Wi-Fi network.

## Facilitator note

The game locks input while the sequence is playing and scrolls to the handoff when all three rounds are complete.
