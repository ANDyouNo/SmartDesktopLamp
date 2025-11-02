# SmartDesktopLamp

A smart RGB desk lamp with HomeKit integration, featuring smooth color transitions and optional visual effects.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--C3-green.svg)
![HomeKit](https://img.shields.io/badge/HomeKit-compatible-orange.svg)

![Lamp in action](https://github.com/ANDyouNo/SmartDesktopLamp/demo.gif)
## Features

- 🎨 Full RGB color control via HomeKit
- 💡 Smooth brightness and color transitions
- 🔘 Physical button control
- 🌈 Visual effects (effects version)
- 📱 Native Apple Home app integration
- ⚡ ESP32-C3 powered

## Hardware Requirements

- **ESP32-C3** microcontroller (LOLIN C3 MINI or compatible)
- **WS2812B LED strip** (8 LEDs recommended)
- **Push button** for manual control
- Power supply suitable for your LED configuration

### Pinout

| Component | Pin |
|-----------|-----|
| WS2812B Data | GPIO 9 |
| Button | GPIO 4 |

## Versions

### `LampFirmareBase` - Basic Version
Simple RGB lamp functionality with smooth transitions. Perfect for everyday use.

**Features:**
- RGB color control
- Brightness adjustment
- Smooth fading transitions
- Physical button toggle

### `LampFirmwareEffects` - Effects Version
Enhanced version with dynamic visual effects.

**Additional Features:**
- **Colorful Twinkle** - Random colorful sparkles (H=352°, S=3%)
- **Aurora** - Flowing aurora borealis effect (H=9°, S=5%)
- **Twinkle** - Warm twinkling effect (H=17°, S=7%)

Activate effects by setting specific HSV values in the Home app.

## Installation

### Prerequisites

1. **Arduino IDE** (version 2.0 or higher recommended)
2. **ESP32 Board Support**
   - Open Arduino IDE
   - Go to `File` → `Preferences`
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to `Tools` → `Board` → `Boards Manager`
   - Search for "esp32" and install **esp32 by Espressif Systems**

3. **Required Libraries**
   - Install via Arduino IDE Library Manager (`Tools` → `Manage Libraries`):
     - **HomeSpan** (by Gregg Orr)
     - **FastLED** (by Daniel Garcia)

### Flashing Instructions

1. **Select Board Configuration**
   - Go to `Tools` → `Board` → `ESP32 Arduino`
   - Select **LOLIN C3 MINI**

2. **Configure Partition Scheme**
   - Go to `Tools` → `Partition Scheme`
   - Select **Huge APP (3MB No OTA/1MB SPIFFS)**
   
   ⚠️ **Important:** The Huge APP partition scheme is required due to the size of HomeSpan and FastLED libraries.

3. **Upload Settings**
   - Upload Speed: `921600`
   - USB CDC On Boot: `Enabled`
   - Flash Mode: `QIO 80MHz`
   - Flash Size: `4MB`

4. **Upload the Code**
   - Connect your ESP32-C3 via USB
   - Select the correct COM port in `Tools` → `Port`
   - Click the Upload button

### First-Time Setup

1. **Monitor Serial Output**
   - Open Serial Monitor (`Tools` → `Serial Monitor`)
   - Set baud rate to `115200`
   - You should see initialization messages

2. **Pairing Code**
   - This code is displayed in the Serial Monitor on startup

3. **HomeKit Pairing**
   - Open Apple Home app on your iOS device
   - Tap "+" → "Add Accessory"
   - Select "More options..."
   - Find "Smart RGB Lamp" in the list
   - Enter the pairing code when prompted
   - Follow the on-screen instructions

## Usage

### HomeKit Control

- **Power:** Turn lamp on/off
- **Brightness:** Adjust from 1-100%
- **Color:** Full HSV color picker
- **Scenes:** Create custom scenes and automations

### Physical Button

- **Short Press:** Toggle lamp on/off
- Maintains last used color and brightness settings

### Effects (Effects Version Only)

To activate effects, set these specific values in the Home app:

| Effect | Hue | Saturation | Brightness |
|--------|-----|------------|------------|
| Colorful Twinkle | 352° | 3% | Any (1-100%) |
| Aurora | 9° | 5% | Any (1-100%) |
| Twinkle | 17° | 7% | Any (1-100%) |

**Note:** Use the color wheel carefully to match these exact values, or create shortcuts/scenes with these settings.

## Customization

### LED Configuration

Change the number of LEDs in the code:

```cpp
#define NUM_LEDS 8  // Change to your LED count
```

### Pairing Code

Modify the pairing code (must be 8 digits):

```cpp
#define PAIRING_CODE "67435891"
```

### Pin Assignment

Adjust pin numbers if needed:

```cpp
#define LED_PIN 9
#define BUTTON_PIN 4
```

### Transition Speed (Basic Version)

Adjust fade speed in the code:

```cpp
const float fadeSpeed = 0.07f;  // Range: 0.01 - 0.5
```

## Technical Details

### Libraries Used

- **HomeSpan 1.9+** - HomeKit accessory protocol
- **FastLED 3.6+** - High-performance LED control

### Memory Requirements

- Flash: ~2.8MB (requires Huge APP partition)
- RAM: ~180KB during operation

### Performance

- LED Update Rate: 20-50ms depending on mode
- Color Transition: Smooth 70ms fade steps
- Button Debounce: 50ms

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Credits

- Built with [HomeSpan](https://github.com/HomeSpan/HomeSpan) by Gregg Orr
- LED control via [FastLED](https://github.com/FastLED/FastLED)

## Support

If you find this project helpful, consider:
- ⭐ Starring the repository
- 🐛 Reporting bugs and suggesting features
- 🔀 Contributing code improvements

---

**Made with ❤️ for the HomeKit community**
