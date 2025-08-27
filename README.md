# Elegoo Arduino R3 TFT MTG Life Counter

A Magic: The Gathering life counter with touchscreen interface for tracking life totals, poison counters, and commander damage for 4 players.

![Demo Video](img/demo.gif)

## Features

- 4 player support
- Touch interface with intuitive controls 
- Tracks:
  - Life totals
  - Poison counters (0-10)
  - Commander damage between players (0-21)
- Clean UI with player color coding
- Reset functionality (hold reset button for 2 seconds)
- Large, easy to read displays

## Hardware Requirements

- [Elegoo UNO R3](https://www.amazon.com/ELEGOO-Board-ATmega328P-ATMEGA16U2-Arduino/dp/B01EWOE0UU)
- [Elegoo 2.8" TFT Touch Screen](https://www.amazon.com/ELEGOO-Screen-Display-Windows-Arduino/dp/B01EUVJYME)

## Libraries Required

- Elegoo_GFX
- Elegoo_TFTLCD
- TouchScreen

## Wiring

### Touch Screen Pins
- YP: A3 
- XM: A2
- YM: 9
- XP: 8

### TFT Control Pins
- LCD_CS: A3
- LCD_CD: A2
- LCD_WR: A1
- LCD_RD: A0
- LCD_RESET: A4

## Setup

1. Install the required libraries in Arduino IDE
2. Connect the TFT screen to Arduino following the pin configuration
3. Upload the code to your Arduino
4. Power up and start tracking your games!

## Usage

- Tap left/right side of player panels to decrease/increase values
- Use sidebar buttons to switch between modes:
  - ❤️ Life totals
  - ☠️ Poison counters
  - ⚔️ Commander damage
- Hold reset button (↻) for 2 seconds to reset all counters

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## Open Tasks

- [ ] Buy a 3D-Printer and print a case
- [ ] Cherry MX switches for better haptic and usability
    - [ ] Design button layout:
        - Life up/down
        - Mode selection
        - Reset button
- [ ] Modify code to support hardware inputs
- [ ] Test and validate button reliability



## License

[MIT](https://choosealicense.com/licenses/mit/)