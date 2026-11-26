# Third-party software

The original Pocket Reef application source and documentation are MIT licensed.
Third-party code and compiled dependencies retain their own licenses.

- Browser bitmap glyphs derive from Adafruit GFX's classic font. Original BSD notice is in `AquariumHome/Adafruit-GFX-font-license.txt` and `licenses/Adafruit-GFX-BSD.txt`.
- Firmware links Adafruit GFX 1.12.6 (BSD), Adafruit ST7735/ST7789 1.11.0 (MIT, see `licenses/Adafruit-ST7735-ST7789.txt`) and Adafruit BusIO 1.17.4 (MIT).
- Arduino-ESP32 3.3.10 is LGPL-2.1-or-later. ESP-IDF components and bundled radio/runtime libraries have their respective licenses. The Arduino-ESP32 source, license tree, Espressif source and dependency build information are linked below. No modified Arduino core or Adafruit library is used. The complete application source and pinned build instructions are provided so the firmware can be rebuilt and relinked with modified libraries.
- The optional web installer loads ESP Web Tools 10.5.2, Apache-2.0, from unpkg. It is not embedded in the offline aquarium page.

Upstream source and license references:

- https://github.com/adafruit/Adafruit-GFX-Library/tree/1.12.6
- https://github.com/adafruit/Adafruit-ST7735-Library/tree/1.11.0
- https://github.com/adafruit/Adafruit_BusIO/tree/1.17.4
- https://github.com/espressif/arduino-esp32/tree/3.3.10
- https://github.com/espressif/esp-idf/tree/v5.5.4
- https://github.com/esphome/esp-web-tools/tree/10.5.2

The visual concept is a small character aquarium. This repository is an independent implementation; it does not contain Pete Cybriwsky's original firmware, another creator's UI assets, or screenshots of their work.
