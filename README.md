# RandomWalkSequencer

The **RandomWalkSequencer** is a MIDI (Musical Instrument Digital Interface) sequencer audio plugin for use in a digital 
audio workstation (DAW) (e.g. Ableton, FL Studio, Reason). The plugin applies the random walk algorithm to generate 
evolving rhythmic and melodic MIDI patterns for use in virtual music production.

![User Interface (UI) of the RandomWalkSequencer.](https://raw.githubusercontent.com/davidfrivas/RandomWalkSeq/master/img/rws-ui.png)

## RandomWalkSequencer Build Instructions

This document explains how to build the RandomWalkSequencer plugin from source for both macOS and Windows platforms.

Directory to plugin source code: [here](./Plugins/RandomWalkSequencer/).

## Prerequisites

### macOS
- Xcode (latest version recommended)
- CMake (version 3.16 or higher)
- JUCE framework (included via CMake)

### Windows
- Visual Studio 2022 (Community Edition is sufficient)
- CMake (version 3.16 or higher)
- JUCE framework (included via CMake)

## Building the Plugin

### Using the Automated Build Scripts

#### For macOS:

1. Open Terminal
2. Navigate to the project directory
3. Make the script executable:
   ```bash
   chmod +x build_release_macos.sh
   ```
4. Run the script:
   ```bash
   ./build_release_macos.sh
   ```

The script will:
- Check if you have CMake 3.16 or higher installed
- Remove existing plugin installations
- Clean the build directory
- Configure CMake in Release mode
- Build VST3 version
- Copy the plugin to the appropriate system directory

#### For Windows:

1. Open Command Prompt (or PowerShell)
2. Navigate to the project directory
3. Run the script:
   ```
   build_release_windows.bat
   ```

The script will:
- Check if you have CMake 3.16 or higher installed
- Remove existing plugin installations
- Clean the build directory
- Detect Visual Studio installation
- Configure CMake with the Visual Studio generator
- Build VST3 version
- Copy the plugin to the VST3 directory

### Manual Build Process

If you prefer to build manually:

#### macOS:

```bash
mkdir -p cmake-build-release
cd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target RandomWalkSequencer_VST3 --config Release
```

#### Windows:

```
mkdir cmake-build-release
cd cmake-build-release
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target RandomWalkSequencer_VST3 --config Release
```

## Installation Directories

### macOS
- VST3: `~/Library/Audio/Plug-Ins/VST3/`

### Windows
- VST3: `%USERPROFILE%\Documents\VST3\`

## Troubleshooting

- **CMake Version**: Ensure you have CMake 3.16 or higher installed. You can check your version with `cmake --version`.
- **Build Errors**: Make sure your development environment is properly set up with the required dependencies.
- **Plugin Not Found by DAW**: Some DAWs require rescanning the plugin directories. Check your DAW's documentation for instructions on how to refresh the plugin list.
- **macOS Security Warnings**: If macOS warns about unknown developer, you may need to allow the plugin in System Preferences > Security & Privacy.

## Reference

Repo template used for plugin: https://github.com/eyalamirmusic/JUCECmakeRepoPrototype/tree/master