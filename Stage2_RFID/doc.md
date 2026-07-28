# Room 2 / Stage 2 — EVA Archive: Administrator Recovery

## Story beat

The smartboard opens EVA's early-memory archive: its creation, its role in civilisation, and humanity gradually treating it as invisible infrastructure. The archive fails before the memory is complete:

`SYSTEM ERROR: ADMIN PRIVILEGES REQUIRED`

## Participant task

1. Enter `123456` on the smartboard archive page.
2. Watch the archive until it halts and displays the RFID instruction.
3. Write `123456` to **Block 4** of the supplied MIFARE Classic RFID card.
4. Scan that card at the RC522 reader.

The OLED reports the number of matching bytes for a wrong card. A correct card restores the administrator memory sector.

## Expected result

The OLED displays **Room 3** and `123456`. This is the next access code and the second recovered key position. The RFID value is not an additional seventh key.

## Files and hardware

- `index.html` — smartboard archive, video gate, and administrator-error prompt.
- `rfid_oled/rfid_oled.ino` — reader and OLED firmware.
- Hardware: ESP32, RC522, SSD1306 I2C OLED, MIFARE Classic card, and a separate RFID writer.

Default wiring: RC522 SS 5 / RST 27; OLED SDA 21 / SCL 22. The card must retain the default Key A: `FF FF FF FF FF FF`.

## Facilitator note

Block 4 is a data block; do not use a sector trailer. Keep the writer ready but do not pre-program the card.
