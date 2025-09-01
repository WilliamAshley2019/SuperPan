# SuperPan

SuperPan is a JUCE-based audio plugin (VST3 and standalone) that offers multiple panning laws for precise stereo control. It supports Linear, -3 dB Constant Power, Square-root, and Balance pan laws, with real-time meters and a visual gain curve.

## Features
- Four panning laws: Linear, -3 dB Constant Power, Square-root, and Balance.
- Smooth parameter transitions to prevent clipping/noise.
- Visual feedback with gain meters and a dynamic curve display.
- Built with JUCE 8, targeting Windows (Debug x64).

## Installation
1. Clone the repository: `git clone https://github.com/WilliamAshley2019/SuperPan`
2. Open `SuperPan.jucer` in Projucer and export to Visual Studio 2022.
3. Build the Debug x64 configuration.
4. Load the VST3 (`Builds/VisualStudio2022/x64/Debug/VST3/SuperPan.vst3`) in your DAW or run the standalone executable.

## Requirements
- JUCE 8
- Visual Studio 2022
- A DAW supporting 64-bit VST3 plugins (for VST3 version)

## Contributing
Contributions are welcome! Please submit issues or pull requests for new pan laws, features, or bug fixes.

## License
As per Steinberg and JUCE licensing requirements SuperPan is GPLv3

The VST3 version is provided for people who do not know how to build from source. It has been tested on my Beelink Ser7 mini PC and runs smoothly as expected.

EULA: Use at your own discretion no warranties are implied if you use this plugin. It was made for Windows 11 24h2. User assumes all risks in using the file, none are known or expected but I have just made this today.
It is a finished Alpha. If anyone has specific functions to add to this feel free to contact me. If you encounter any bugs also feel free to contact me.
