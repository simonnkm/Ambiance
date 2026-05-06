# Ambiance

Ambiance is a solar-powered scheduled audio playback system built around an STM32 Nucleo development board. The system is designed for remote outdoor use and supports scheduled playback of audio tracks through a speaker system.

The project includes embedded firmware, a desktop GUI, hardware documentation, PCB manufacturing files, and assembly materials.

## Repository Structure

```text
Ambiance/
├── Ambiance-main/      Embedded STM32 firmware and device-side code
├── Ambiance_GUI/       Desktop GUI for UART/Bluetooth control
├── dist/               Packaged application output
```

## Main Components

### Embedded Firmware

The embedded firmware runs on the STM32 Nucleo board. It controls scheduled audio playback, button input, OLED menu navigation, Bluetooth/UART communication, real-time clock behavior, and DFPlayer Mini track playback.

### Desktop GUI

The desktop GUI is located in:

```text
Ambiance_GUI/
```

The GUI connects to the device through UART or Bluetooth and provides controls for:

* Device connection
* Volume
* Duty cycle
* Manual track selection
* Schedule creation
* Schedule import/export
* Log download

See:

```text
Ambiance_GUI/README.md
```

### Hardware and Assembly

The hardware and assembly instructions are provided separately as a Word document. That document includes the physical build steps, wiring, DFPlayer Mini file setup, FAT drive sorting behavior, bill of materials, Gerber files, and assembly notes.

## OLED Screen Options

The Ambiance device can also be controlled directly from the onboard buttons and OLED screen.

The OLED menu includes:

### Play Track

Used to immediately play a specific audio track from a selected folder.

The user selects:

* Folder number
* File number

The device then sends the selected track command to the DFPlayer Mini.

### Schedule

Used to create scheduled playback entries directly on the device.

A schedule includes:

* Month
* Start day
* End day
* Start time
* Stop time
* Folder number
* File number

A month value of `0` means the schedule repeats every month.

### Set Date and Time

Used to set the device's real-time clock.

The real-time clock keeps the current time so the device can trigger scheduled playback correctly.

### More Options

Used for device-level controls and maintenance options.

This menu includes:

* Duty cycle adjustment
* Next track
* Previous track
* Clear schedule

### Volume Control

The left and right buttons adjust the speaker volume.

## DFPlayer Mini File Numbering

The DFPlayer Mini uses numbered folders and numbered audio files.

Recommended folder format:

```text
01
02
03
```

Recommended file format:

```text
0001.mp3
0002.mp3
0003.mp3
```

Example SD card layout:

```text
/01/0001.mp3
/01/0002.mp3
/02/0001.mp3
```

Folder numbers are used by the firmware and GUI when selecting tracks. File numbers are used to choose the specific audio file inside that folder.
Avoid folder 4 track 7, it is not going to play. A placeholder should be fine.

## DFPlayer Mini FAT Drive Sorting

The DFPlayer Mini reads files based on how they are stored on the FAT-formatted SD card. File order can depend on the order files are copied onto the card, not just the visible alphabetical order shown on a computer.

To keep playback reliable:

1. Format the SD card as FAT32.
2. Use numbered folders.
3. Use numbered MP3 files.
4. Copy files onto the SD card in the intended order.
5. Avoid renaming, deleting, and re-adding files after copying.

Incorrect file ordering can cause the DFPlayer Mini to play the wrong track number.

## Hardware Files

The hardware package includes:

* Gerber files
* Bill of materials
* PCB assembly notes
* Hardware and assembly instructions
* DFPlayer Mini SD card setup instructions

## Firmware Setup

See the README inside:

```text
Ambiance-main/
```

for STM32 firmware setup and flashing instructions.

## GUI Setup

See the README inside:

```text
Ambiance_GUI/
```

for GUI installation, running, and packaging instructions.
