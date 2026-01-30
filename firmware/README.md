# Glass Drop – Firmware

This folder contains the Arduino firmware for the Glass Drop coaster.

## Hardware
- Arduino Nano
- VCNL4040 proximity sensor
- TM1637 4-digit display
- WS2812B LEDs
- Active buzzer
- One button

## Upload
1. Open `glass_drop.ino` in the Arduino IDE
2. Select **Board: Arduino Nano**
3. Select **Processor: ATmega328P**
4. Upload

## Modding
- New game modes can be added in the `GameState` enum
- Main logic is handled in `loop()` using `switch(state)`
- Sensor logic is isolated in `updateGlassState()`

