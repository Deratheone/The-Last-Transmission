# Room 5 / Stage 5 — EVA Encrypted Memory Cache

## Story beat

This data facility preserves a damaged exchange between EVA and the unidentified signal that appeared at 03:17:42 UTC. The cache is close to collapse, so only one short encrypted phrase can load the conversation.

## Participant task

1. Join `EVA-CACHE-5` with password `RY204200`.
2. Open the ESP32 IP address shown in Serial Monitor.
3. Reverse the `+3` Caesar shift in `HYD UHPHPEHUV WLPH`.
4. Enter the decrypted phrase: `EVA REMEMBERS TIME`.

## Expected result

The recovered exchange says that EVA hid something the fragmentation protocol could not erase. It reveals a hidden synchronization cache in **Room 6** and its access key, `NOTDIS`.

## Files and hardware

- `encryption_game/encryption_game.ino` — ESP32 access point and embedded terminal.
- `webserver.html` — standalone browser preview.
- Hardware: one ESP32 and a phone/laptop connected to its Wi-Fi network.

## Facilitator note

Allow the recovered conversation to remain on screen. It should make Room 6 feel like EVA's secret contingency, not an ordinary official node.
