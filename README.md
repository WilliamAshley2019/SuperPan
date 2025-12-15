# SuperPan

SuperPan is a JUCE-based VST3 audio plugin that offers multiple panning laws for precise stereo control. It supports Linear, -3 dB Constant Power, Square-root, and Balance pan laws, with real-time meters and a visual gain curve.

## Features
- Four panning laws: Linear, -3 dB Constant Power, Square-root, and Balance.
- Smooth parameter transitions to prevent clipping/noise.
- Visual feedback with gain meters and a dynamic curve display.
- Built with JUCE 8, targeting Windows (Debug x64).


## Requirements
- JUCE 8 (last compiled in Juce 8.0.11), + VST3 SDK the .jucer file is now provided however, if building from scatch in juce use plugin basics + dsp. 
- Visual Studio 2022 last compiled in v17 or similar environment to build.
- A DAW supporting 64-bit VST3 plugins (for VST3 version) to test.

## Contributing
Contributions are welcome! Please submit issues or pull requests for new pan laws, features, or bug fixes.

## License
As per Steinberg and JUCE licensing requirements SuperPan is GPLv3

It has been tested on my Beelink Ser7 mini PC and runs smoothly as expected. Version 2 aims to improve CPU processing by optimizing SIMD block processing rather than per sample processing.

EULA: Use at your own discretion no warranties are implied if you use this plugin. It was made for Windows 11 24h2, and version 2 was built and tested in 25h2. User assumes all risks in using the file, none are known or expected but I have just made this today.
It is a finished Alpha. If anyone has specific functions to add to this feel free to contact me. If you encounter any bugs also feel free to contact me.



LAST UPDATED 2025-12-15 ~4:20AM EST
