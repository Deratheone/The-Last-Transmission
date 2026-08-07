# Room 4 / Stage 4 — EVA Morse Archive

## Story beat

Only a weak Morse trace remains at this node. The first touch wakes the damaged archive: green indicates a live trace, red indicates corruption, and the buzzer acts as an alarm.

## Participant task

Enter `TIMEIS` using the touch sensor and the desk's Morse reference table:

- short press = dot
- long press = dash
- 700 ms pause = finish one digit

Each correct digit receives a green confirmation. A wrong digit flashes red and resets the entry.

## Expected result

After all six characters are correct, the device sends the next handoff through the green LED in Morse: **Room 5** and `RY2042`.

## Files and hardware

- `morse_game/morse_game.ino` — uploadable touch/Morse/RGB/buzzer firmware.
- Hardware: ESP32, digital touch sensor, active-HIGH RGB LED, passive buzzer, and Morse reference table.

Default pins: touch 14; buzzer 25; red 26; green 27; blue 33. Invert `setRGB` if using an active-LOW/common-anode LED.

## Facilitator note

Keep the Morse table visible. The group should decode the green LED output themselves rather than being told the Room 5 code.
