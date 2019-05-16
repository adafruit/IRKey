# Firmware burned into the Adafruit IRKey IR Remote -> Keyboard adapter

<a href="http://www.adafruit.com/products/1561"><img src="assets/image.jpg?raw=true" width="500px"></a>

Check the Makefile for fuses and avrdude commands.

The IRKey adds an IR remote receiver to any computer, laptop, tablet...any computer with a USB port. This little board slides into any USB A port, and shows up as an every-day USB keyboard. The onboard ATiny85 microcontroller listens for IR remote signals and converts them to keypresses. We bundle this with our remote with 21 buttons so it controls nearly anything you want. For ultra-hackers, you can re-program the firmware to customize it however you wish, [check out the GitHub repository for the firmware.](https://github.com/adafruit/IRKey)

IRKey is designed for use with our mini IR remote, but won't work with any other remote... so be aware it isn't "plug & play" with your home remote!

It's great for controlling an XBMC computer, but also nice when you want to make a clicker for watching videos or playing music on your computer or laptop. Since it's just a USB keyboard, no drivers are required for any operating systems.

There are two modes: ASCII and Multimedia key. ASCII mode is default, there's a single blink on startup to let you know. The output is all ASCII-type characters that any keyboard can generate. Multimedia mode has two blinks on startup, and sends MM key's as seen on some keyboards. This will let you do stuff like control your computer's speaker volume at any time, or iTunes, even if it's in the background.

- "Vol-" -> '-' in ASCII mode or 'Volume down' in Multimedia Key mode
- "Vol+" -> '=' in ASCII mode or 'Volume up' in Multimedia Key mode
- "Play/pause" -> ' ' (space) in ASCII mode or 'Play/Pause' in Multimedia Key mode
- "Setup" -> Escape key in ASCII mode or 'Menu" in Multimedia Key mode
- "Stop/Mode" -> 'x' in ASCII mode or 'Stop' in Multimedia Key mode
- Up/Down/Left/Right -> Arrow keys in ASCII mode or Volume Up/Down and Prev/Next track in Multimedia Key mode
- Enter/Save -> Enter key
- Reverse -> Backspace key
- 0 thru 9 -> '0' thru '9'

You can switch between modes by waiting until the IRKey is plugged in and working and pressing down the mini button for one second. The LED will blink to show you that the modes have switched.

## License

Adafruit invests time and resources providing this open source design, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Frank Zhao, for Adafruit Industries
(c) 2013 
Based on V-USB

Licensed under GPL v2

All text above must be included in any redistribution
