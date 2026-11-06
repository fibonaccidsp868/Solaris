# Solaris Saturator (VST3) - source project

## Get the plugin
**Easiest (no compiler needed):** push this folder to a GitHub repo -> Actions tab -> "Build VST3" ->
download the `Solaris-VST3-<os>` artifact. Copy the `.vst3` folder to:
- Windows: `C:\Program Files\Common Files\VST3`
- macOS: `~/Library/Audio/Plug-Ins/VST3`
- Linux: `~/.vst3`

**Local build:** install CMake + a C++17 compiler, then run `build.bat` (Windows, as Administrator) or `./build.sh`.
It downloads JUCE 8.0.6 automatically and installs the VST3 for you.

## Signal flow
In -> 4x oversample (linear-phase FIR) -> Input gain -> [Single: thermal tanh ADAA saturator | Multiband: 5x LR4 bands, each saturated + level] ->
DC block -> Clarity (3.8 kHz) -> Air (Maag-style wide shelves + exciter) -> Output gain -> Soft clip -> 4x down -> Dry/Wet.
Crossovers: 120 / 500 / 2000 / 6500 Hz. Reported latency is compensated for dry/wet.

## Status
Upload the folder structure as-is (Source/ and .github/ included). The build also accepts the .cpp files placed next to CMakeLists.txt.
Not yet auditioned - treat as a first release candidate.
