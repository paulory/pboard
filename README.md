# pboard

A small guitar pedalboard for macOS: VST3, AU and a standalone app.

Signal chain: **IN > GNAW (distortion) > SWIRL (chorus) > PB-50 amp > IR CAB > OUT**

| Block | What it is |
|---|---|
| **GNAW** | Component-level model of the classic RAT-style circuit: LM308 op-amp gain stage with its real gain network, finite gain-bandwidth and slow slew rate, clipping diodes solved per sample, and the FILTER RC network. Diode switch: **SI** (1N914, classic), **LED** (Turbo style), **GE** (1N34A, You-Dirty style). Runs 4x oversampled. |
| **SWIRL** | Analog BBD-style chorus (CE-2 flavour): one short delay line swept by a rounded-triangle LFO, band-limited like a bucket-brigade chip. MIX at 10 is the classic equal wet/dry blend. |
| **PB-50** | Two-channel amp. **CLEAN**: American-style, Fender tone stack, lots of headroom. **CRUNCH**: British-style cascaded gain stages, bright cap, Marshall tone stack. Both tone stacks are the real passive circuits (derived by nodal analysis), so the knobs interact like on the real thing. |
| **IR CAB** | Convolution cab. Ships with a built-in synthetic 4x12 voicing; **LOAD IR** takes any `.wav` / `.aif` impulse response. The IR path is saved with the session. |

## Install (from GitHub Actions)

1. Open the **Actions** tab, pick the latest green **Build macOS** run, download **pboard-macos**.
2. Unzip and copy:
   - `pboard.app` to `/Applications`
   - `pboard.vst3` to `~/Library/Audio/Plug-Ins/VST3`
   - `pboard.component` to `~/Library/Audio/Plug-Ins/Components`
3. The builds are ad-hoc signed (not notarised), so clear the download quarantine once:
   ```sh
   xattr -dr com.apple.quarantine /Applications/pboard.app \
     ~/Library/Audio/Plug-Ins/VST3/pboard.vst3 \
     ~/Library/Audio/Plug-Ins/Components/pboard.component
   ```

Pushing a tag like `v0.1.0` also publishes the zip as a GitHub Release.

## Standalone tips

- Open **Options > Audio/MIDI Settings**, pick your interface and the input your guitar is on. pboard uses the **first input channel**.
- JUCE's standalone mutes the input by default to avoid feedback through laptop speakers. Untick **Mute audio input** in the same settings window when using headphones or an interface.
- Allow microphone access when macOS asks (that is how macOS gates audio inputs).

## Build locally

```sh
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release
ctest --test-dir build -C Release      # offline DSP test
```

JUCE 8 is downloaded automatically by CMake.

## Code map

- `Source/dsp/RatPedal.h` distortion circuit model
- `Source/dsp/Chorus.h` BBD chorus
- `Source/dsp/ToneStack.h`, `Source/dsp/AmpSim.h` amp channels
- `Source/dsp/DefaultCab.h` built-in cab IR
- `Source/PluginProcessor.*` parameters, oversampling, chain, state
- `Source/PluginEditor.*` pedalboard UI
- `tests/dsp_test.cpp` renders every module and setting, fails on NaN, silence or runaway levels

Names on the board (GNAW, SWIRL, PB-50) are original; no third-party brands or artwork are used.

JUCE is licensed under AGPLv3 / commercial terms; check the JUCE licence before distributing builds publicly.
