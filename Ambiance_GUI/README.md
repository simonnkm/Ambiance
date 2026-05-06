# Ambiance GUI

This GUI application allows users to connect to STM32-based audio devices via UART or Bluetooth to control playback, volume, schedules, and download logs. It supports macOS and Windows, and can be packaged into standalone applications.

## Features
- Bluetooth and UART selection and connection
- Volume and duty cycle control
- Schedule configuration with overlap validation
- Log downloading and saving
- Real-time UART debug display

## GUI Components

The Ambiance GUI is organized into four main sections: connection controls, playback controls, scheduler controls, and the output log.

### Connection Controls

The connection area lets the user choose between UART and Bluetooth.

For UART, the GUI provides:

* Baud rate selection
* Serial port list
* Refresh button
* Connect and disconnect buttons

For Bluetooth, the GUI provides:

* Scan for Devices button
* Discovered Devices list
* Connect and disconnect buttons

The connection status indicator shows whether the system is disconnected, connecting, or connected.

### Bluetooth Device List

The Bluetooth scan displays discovered Bluetooth devices using the device name and the end of the device address. This makes it easier to tell apart devices with similar or missing names.

The current WPS speaker filter does not remove non-speaker devices. It returns every discovered Bluetooth device. Compatibility is checked after the user selects a device and attempts to connect.

During connection, the GUI verifies that the selected device has the required Ambiance BLE service and characteristics. Devices without the required service are rejected as incompatible.

### Playback Controls

The control panel includes:

* Volume slider
* Duty cycle slider
* Manual folder number input
* Manual file number input
* Send Track button

The volume slider sets the playback volume.

The duty cycle slider controls the playback duty cycle percentage.

The track selection fields allow the user to send a specific folder and file number to the DFPlayer Mini.

### Scheduler Controls

The scheduler allows timed playback entries to be created and sent to the device.

Each schedule entry contains:

* Month
* Start day
* End day
* Start hour and minute
* Stop hour and minute
* Folder number
* File number

The month field supports `0` for monthly repeating schedules.

The minute fields use 15-minute increments: `00`, `15`, `30`, and `45`.

The GUI checks schedule entries before adding them to the queue. It rejects invalid day ranges, invalid time ranges, invalid minutes, and overlapping schedules.

### Schedule Actions

The scheduler includes the following actions:

* Add Entry
* Send Schedules
* Clear Queue
* Export Schedules
* Import Schedules

`Add Entry` adds the current schedule to the queue.

`Send Schedules` sends all queued schedules to the connected device.

`Clear Queue` removes all queued schedules from the GUI.

`Export Schedules` saves the queued schedules to a text file.

`Import Schedules` loads schedules from a saved text file and validates them before adding them to the queue.

### Log and Output Window

The output window displays connection messages, Bluetooth/UART communication, schedule actions, errors, and debug messages.

The GUI includes:

* Download Log
* Clear Output

`Download Log` requests the device log and saves it as a text file.

`Clear Output` clears the visible log window.


---



## If want to mini standalone Python environment that just runs
### MacOS `.app` Bundle

#### Step 1: Install Xcode if you do not have it installed already

```bash
xcode-select --install
```

#### Step 2: Install Required Dependencies

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

```bash
pip install pyinstaller pyobjc-core pyobjc-framework-CoreBluetooth
```

#### Step 2: Generate `.spec` file

```bash
pyi-makespec Ambiance_GUI.py
```

#### Step 3: Modify the generated `Ambiance_GUI.spec`
Open it using TextEdit via:

```bash
open -a TextEdit Ambiance_GUI.spec
```

Replace all of the text in the file with:

```python
 -*- mode: python ; coding: utf-8 -*-

from PyInstaller.utils.hooks import collect_submodules, collect_data_files

a = Analysis(
    ['Ambiance_GUI.py'],
    pathex=['.'],
    binaries=[],
    datas=collect_data_files('serial') + collect_data_files('bleak'),
    hiddenimports=collect_submodules('serial') 
                  + collect_submodules('bleak.backends')
                  + ['serial.tools.list_ports', 'bleak.backends.corebluetooth'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)

pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='Ambiance_GUI',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='Ambiance_GUI',
)

app = BUNDLE(
    coll,
    name='Ambiance.app',
    icon='Ambiance.icns'
)
```

Save these edits.

Make sure `Ambiance.icns` is in the project folder.

---
#### Step 4: Build the App (Removes Previous Builds)

```bash
rm -rf build dist
source .venv/bin/activate
python -m PyInstaller --clean -y Ambiance_GUI.spec
```

Find your `.app` inside the `dist/` folder or open with:

```bash
./dist/Ambiance.app/Contents/MacOS/Ambiance_GUI
```

---
#### Step 5: Clean up build artifacts

After building, clean the unnecessary folders:

**macOS or Linux:**
```bash
rm -rf build/ __pycache__/ *.spec
```

### Windows `.exe` Executable

#### Step 1: Install PyInstaller

```bash
pip install pyinstaller
```

#### Step 2: Create Windows Executable

```bash
pyinstaller --onefile --noconsole --icon=Ambiance.ico Ambiance_GUI.py
```

The output `.exe` file will be found in the `dist/` directory.

---
#### Step 3: Clean up build artifacts

After building, clean the unnecessary folders:

**Windows (CMD):**
```cmd
rmdir /s /q build
del /q *.spec
```

---

## If want to run the .py script or running/modifying
### Requirements

To install the dependencies:

```bash
pip install -r requirements.txt
```

#### `requirements.txt` contains:
```txt
pyserial
bleak
tk
```

---

### Running from Source

To run the GUI manually (Python 3.10+ required):

```bash
python Ambiance_GUI.py
```

---

## Author
**Jaspreet Singh**  
[https://github.com/jsingh08](https://github.com/jsingh08)

**Simon Nakarmi**
[https://github.com/simonnkm](https://github.com/simonnkm)
