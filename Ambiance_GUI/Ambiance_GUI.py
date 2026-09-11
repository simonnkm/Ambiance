"""
Ambiance GUI Application

This application provides a graphical interface for controlling audio playback devices
via Bluetooth or UART connections. It supports:
- Device connection management
- Audio control (volume, track selection)
- Schedule management
- Logging and monitoring
"""

import re
import sys
import serial
import asyncio
import threading
import serial.tools.list_ports
from bleak import BleakScanner, BleakClient
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from datetime import datetime


class AmbianceGUI(tk.Tk):
    """
    Main GUI application class for the Ambiance GUI.
    
    This class handles all UI elements, device connections, and user interactions.
    It supports both Bluetooth and UART connections for device control.
    """
    
    def __init__(self):
        """Initialize the GUI application and set up all UI components."""
        super().__init__()

        # Add protocol handler for window closing
        self.protocol("WM_DELETE_WINDOW", self.on_closing)

        self.state('zoomed')


        # Set up the main window
        self.title("Ambiance GUI") 

        # Create main frame
        self.frame = ttk.Frame(self)
        self.frame.pack(fill="both", expand=True, padx=10, pady=7)

        # Add connection status frame at the top
        self._setup_connection_status()
        
        # Initialize connection type selection
        self._setup_connection_type()
        
        # Initialize device selection and connection controls
        self._setup_device_controls()
        
        # Initialize audio control section
        self._setup_audio_controls()
        
        # Initialize scheduler section
        self._setup_scheduler()
        
        # Initialize log and output section
        self._setup_log_section()
        
        # Initialize asyncio event loop for Bluetooth operations
        self._setup_event_loop()
        
        # Initialize connection variables
        self._init_connection_vars()

    def _setup_connection_status(self):
        """Set up the connection status display at the top of the window."""
        # Connection status is now integrated into the connection frame
        pass

    def _setup_connection_type(self):
        """Set up the connection type selection (Bluetooth/UART) controls."""
        # Initialize connection type variable
        self.connection_type = tk.StringVar()
        self.connection_type.set("UART")  # Default to UART

    def _setup_device_controls(self):
        """Set up the device selection and connection controls."""
        # Create frame for connection label and status
        connection_header = ttk.Frame(self.frame)
        connection_header.pack(fill="x", pady=(5, 0), padx=10)
        
        # Connection status on the right
        self.connection_status_label = ttk.Label(
            connection_header,
            text="● Disconnected",
            foreground="red",
            font=("Arial", 12, "bold")
        )
        self.connection_status_label.pack(side=tk.LEFT)
        
        # Create main connection frame (without label since we have it above)
        connection_frame = ttk.Frame(self.frame)
        connection_frame.pack(fill="x", pady=(0, 5), padx=10)

        # Create inner frame for the two connection sections
        connection_sections = ttk.Frame(connection_frame)
        connection_sections.pack(fill="x", pady=7, padx=10)

        # Set up Bluetooth controls
        self._setup_bluetooth_controls(connection_sections)
        
        # Set up UART controls
        self._setup_uart_controls(connection_sections)

    def _setup_bluetooth_controls(self, parent_frame):
        """Set up the Bluetooth device selection and connection controls."""
        # Create labeled frame for Bluetooth controls with radio button
        bluetooth_frame = ttk.LabelFrame(parent_frame)
        bluetooth_frame.pack(side=tk.LEFT, fill="both", expand=True, padx=(0, 10))

        # Add radio button to the frame label area
        self.bluetooth_button = ttk.Radiobutton(
            bluetooth_frame,
            text="Bluetooth",
            variable=self.connection_type,
            value="Bluetooth",
            command=self.uart_button_toggled
        )
        self.bluetooth_button.pack(anchor="nw", padx=10, pady=(5, 0))

        # Scan / Connect / Disconnect all share one row.
        scan_connect_frame = ttk.Frame(bluetooth_frame)
        scan_connect_frame.pack(pady=7, padx=10, fill="x")

        self.scan_button = ttk.Button(
            scan_connect_frame,
            text="Scan for Devices",
            command=self.start_scan_devices
        )
        self.scan_button.pack(side=tk.LEFT, padx=(0, 5), expand=True, fill="x")

        # Connect button
        self.bluetooth_connect_button = ttk.Button(
            scan_connect_frame,
            text="Connect",
            command=self.connect_to_bluetooth
        )
        self.bluetooth_connect_button.pack(side=tk.LEFT, padx=5, expand=True, fill="x")

        # Disconnect button
        self.bluetooth_disconnect_button = ttk.Button(
            scan_connect_frame,
            text="Disconnect",
            command=self.disconnect_device,
            state=tk.DISABLED
        )
        self.bluetooth_disconnect_button.pack(side=tk.LEFT, padx=(5, 0), expand=True, fill="x")

        # Device list label
        self.devices_label = ttk.Label(bluetooth_frame, text="Discovered Devices:")
        self.devices_label.pack(padx=10)

        # Device listbox
        self.devices_listbox = tk.Listbox(bluetooth_frame, height=5)
        self.devices_listbox.pack(pady=5, padx=10, fill="x")

        # Force Time Sync / Skip Auto Time Sync share a row here since both
        # are about what happens when this device connects, not the
        # schedule (moved from the Scheduler's Time Range section).
        time_sync_frame = ttk.Frame(bluetooth_frame)
        time_sync_frame.pack(pady=5, padx=10, fill="x")

        # Manual time sync - the connected device's clock is normally set
        # automatically on connect, but that happens silently; this lets you
        # force it on demand and see the real success/failure of the write
        # (see force_time_sync) instead of just trusting the auto-sync.
        self.force_time_sync_button = ttk.Button(
            time_sync_frame,
            text="Force Time Sync",
            command=self.force_time_sync
        )
        self.force_time_sync_button.pack(side=tk.LEFT, padx=(0, 5), expand=True, fill="x")
        self.force_time_sync_button.config(state=tk.DISABLED)

        # Skip-auto-sync toggle - the auto time sync fires the instant a
        # connection is established, before there's any chance to click
        # Check Time first, so there's no way to see the device's own
        # unmodified RTC reading unless this is off for that one connect.
        # Leave it checked normally; only uncheck it to diagnose whether the
        # device's clock is drifting/resetting on its own between connects.
        self.skip_auto_time_sync_var = tk.BooleanVar(value=False)
        self.skip_auto_time_sync_check = ttk.Checkbutton(
            time_sync_frame,
            text="Skip auto time sync on next connect (diagnostic)",
            variable=self.skip_auto_time_sync_var
        )
        self.skip_auto_time_sync_check.pack(side=tk.LEFT, padx=(5, 0))

    def _setup_uart_controls(self, parent_frame):
        """Set up the UART port selection and connection controls."""
        # Create labeled frame for UART controls with radio button
        uart_frame = ttk.LabelFrame(parent_frame)
        uart_frame.pack(side=tk.LEFT, fill="both", expand=True, padx=(10, 0))

        # Add radio button to the frame label area
        self.uart_button = ttk.Radiobutton(
            uart_frame,
            text="UART",
            variable=self.connection_type,
            value="UART",
            command=self.uart_button_toggled
        )
        self.uart_button.pack(anchor="nw", padx=10, pady=(5, 0))

        # Baud rate controls
        self._setup_baudrate_controls(uart_frame)

        # Port selection controls
        self._setup_port_controls(uart_frame)

        # Connection buttons
        self._setup_connection_buttons(uart_frame)

    def _setup_baudrate_controls(self, parent_frame):
        """Set up the baud rate selection controls."""
        baud_frame = ttk.Frame(parent_frame)
        baud_frame.pack(pady=5)

        # Baud rate label
        self.baudrate_label = ttk.Label(baud_frame, text="Baud Rate:")
        self.baudrate_label.pack(side=tk.LEFT, padx=10)

        # Baud rate entry
        self.baudrate_var = tk.IntVar(value=9600)  # Default baud rate
        self.baudrate_entry = ttk.Entry(
            baud_frame,
            textvariable=self.baudrate_var,
            width=10
        )
        self.baudrate_entry.pack(side=tk.LEFT, padx=5)

    def _setup_port_controls(self, parent_frame):
        """Set up the serial port selection controls."""
        # Port list label
        self.serial_ports_label = ttk.Label(parent_frame, text="Serial Port:")
        self.serial_ports_label.pack(pady=5)

        # Port listbox
        self.serial_listbox = tk.Listbox(parent_frame, height=5)
        self.serial_listbox.pack(pady=5, fill="x")

    def _setup_connection_buttons(self, parent_frame):
        """Set up the connection and refresh buttons."""
        button_frame = ttk.Frame(parent_frame)
        button_frame.pack(pady=7)

        # Connect button
        self.uart_connect_button = ttk.Button(
            button_frame,
            text="Connect",
            command=self.connect_to_uart
        )
        self.uart_connect_button.pack(side=tk.LEFT, padx=10)

        # Disconnect button
        self.uart_disconnect_button = ttk.Button(
            button_frame,
            text="Disconnect",
            command=self.disconnect_device,
            state=tk.DISABLED
        )
        self.uart_disconnect_button.pack(side=tk.LEFT, padx=10)

        # Refresh button
        self.refresh_button = ttk.Button(
            button_frame,
            text="Refresh",
            command=self.refresh_serial_ports
        )
        self.refresh_button.pack(side=tk.LEFT, padx=10)

        # Populate initial port list
        self.populate_serial_ports()

    def _setup_audio_controls(self):
        """Set up the audio control section."""
        # Create frame for audio controls
        control_frame = ttk.LabelFrame(self.frame, text="Controls")
        control_frame.pack(fill="x", pady=7, padx=10)

        # Single row - All controls with proper spacing
        controls_row = ttk.Frame(control_frame)
        controls_row.pack(fill="x", pady=(10, 5), padx=10)

        # Volume control (left side)
        volume_section = ttk.LabelFrame(controls_row, text="Volume")
        volume_section.pack(side=tk.LEFT, fill="both", expand=True, padx=(0, 10))

        # Volume input field
        volume_input_frame = ttk.Frame(volume_section)
        volume_input_frame.pack(pady=7, padx=10)
        self.volm = tk.IntVar()
        # length=280 gives ~2.8 pixels per unit (default length is only
        # ~100px for this 0-100 range, i.e. ~1px/unit) - too fine to reliably
        # click an exact value like 20, 30, 70 by mouse; resolution=1 makes
        # the 1-unit step explicit rather than relying on the tk.Scale default.
        self.volume_input = tk.Scale(volume_input_frame, orient='horizontal', variable=self.volm, from_=0, to=100, resolution=1, length=280, showvalue=1)
        self.volume_input.pack()
        
        # Volume set button below
        self.volume_set_button = ttk.Button(
            volume_section,
            text="Set",
            command=lambda: self.set_volume(self.volm.get())
        )
        self.volume_set_button.pack(pady=(0, 10), padx=10)

        # Duty cycle control (center)
        duty_section = ttk.LabelFrame(controls_row, text="Duty Cycle")
        duty_section.pack(side=tk.LEFT, fill="both", expand=True, padx=(0, 10))

        # Duty cycle input field
        duty_input_frame = ttk.Frame(duty_section)
        duty_input_frame.pack(pady=7, padx=10)
        self.duty_cyc = tk.IntVar()
        # Same precision fix as the volume slider above - see that comment.
        self.duty_cycle_input = tk.Scale(duty_input_frame, orient='horizontal', variable=self.duty_cyc, from_=0, to=100, resolution=1, length=280, showvalue=1)
        self.duty_cycle_input.pack()
        
        # Duty cycle set button below
        self.duty_cycle_button = ttk.Button(
            duty_section,
            text="Set",
            command=lambda: self.set_duty_cycle(self.duty_cyc.get())
        )
        self.duty_cycle_button.pack(pady=(0, 10), padx=10)

        # Folder and File control (right side)
        track_section = ttk.LabelFrame(controls_row, text="Track Selection")
        track_section.pack(side=tk.LEFT, fill="both", expand=True)

        # Folder and File labels and inputs on one line
        track_input_frame = ttk.Frame(track_section)
        track_input_frame.pack(pady=7, padx=10)
        
        ttk.Label(track_input_frame, text="Folder #:").pack(side=tk.LEFT, padx=(0, 5))
        self.manual_folder_entry = ttk.Entry(track_input_frame, width=6)
        self.manual_folder_entry.pack(side=tk.LEFT, padx=(0, 10))

        ttk.Label(track_input_frame, text="File #:").pack(side=tk.LEFT, padx=(0, 5))
        self.manual_file_entry = ttk.Entry(track_input_frame, width=6)
        self.manual_file_entry.pack(side=tk.LEFT)
        
        # Send Track button below
        self.track_send_button = ttk.Button(
            track_section,
            text="Send Track",
            command=lambda: self.send_folder_file(
                self.manual_folder_entry.get(),
                self.manual_file_entry.get()
            )
        )
        self.track_send_button.pack(pady=(0, 10), padx=10)

    def _setup_scheduler(self):
        """Set up the scheduler section for timed playback."""
        # Create frame for scheduler
        scheduler_frame = ttk.LabelFrame(self.frame, text="Scheduler")
        scheduler_frame.pack(side=tk.LEFT, fill="both", anchor = 'center', expand=True, padx=(0, 10))

        # Create main container for better organization
        main_container = ttk.Frame(scheduler_frame)
        main_container.pack(pady=7, padx=10)

        # Top row - Date, Time, and Audio File controls
        controls_row = ttk.Frame(main_container)
        controls_row.pack(fill="x", pady=(0, 10))

        # Date Range section (left)
        self._setup_date_section(controls_row)
        
        # Time Range section (center)
        self._setup_time_section(controls_row)

        # Audio File section (right)
        self._setup_file_section(controls_row)

        # Bottom row - Action buttons
        actions_row = ttk.Frame(main_container)
        actions_row.pack(fill="x")
        
        # Action buttons section
        self._setup_action_buttons(actions_row)

    def _setup_date_section(self, parent_frame):
        """Set up the date selection controls with better organization."""
        # Create labeled frame for date controls
        date_frame = ttk.LabelFrame(parent_frame, text="Date Range")
        date_frame.pack(side=tk.LEFT, fill="both", expand=True, padx=(0, 10))

        # Month and day controls in a grid
        date_grid = ttk.Frame(date_frame)
        date_grid.pack(pady=7, padx=10)

        # Month selection
        ttk.Label(date_grid, text="Month:").grid(row=0, column=0, padx=(0, 5), pady=5, sticky="e")
        self.month_entry = ttk.Spinbox(date_grid, from_ = 0, to = 12,width = 8, wrap=True)
        self.month_entry.grid(row=0, column=1, padx=(0, 15), pady=5, sticky="w")

        # Start day selection
        ttk.Label(date_grid, text="Start Day:").grid(row=1, column=0, padx=(0, 5), pady=5, sticky="e")
        self.start_day_entry = ttk.Entry(date_grid, width=8)
        self.start_day_entry.grid(row=1, column=1, padx=(0, 15), pady=5, sticky="w")

        # End day selection
        ttk.Label(date_grid, text="End Day:").grid(row=2, column=0, padx=(0, 5), pady=5, sticky="e")
        self.end_day_entry = ttk.Entry(date_grid, width=8)
        self.end_day_entry.grid(row=2, column=1, padx=(0, 15), pady=5, sticky="w")

        # Repeat information label
        self.repeat_info_label = ttk.Label(
            date_frame,
            text="(Enter Month = 0 for monthly repeating schedules)",
            foreground="gray",
            font=("Arial", 8)
        )
        self.repeat_info_label.pack(pady=(0, 5))

    def _setup_time_section(self, parent_frame):
        """Set up the time selection controls with better organization."""
        # Create labeled frame for time controls
        time_frame = ttk.LabelFrame(parent_frame, text="Time Range")
        time_frame.pack(side=tk.LEFT, fill="both", expand=True, padx=(0, 10))

        # Time controls in a grid
        time_grid = ttk.Frame(time_frame)
        time_grid.pack(pady=7, padx=10)

        # Start time controls
        ttk.Label(time_grid, text="Start Time:").grid(row=0, column=0, padx=(0, 5), pady=5, sticky="e")
        start_time_frame = ttk.Frame(time_grid)
        start_time_frame.grid(row=0, column=1, padx=(0, 15), pady=5, sticky="w")
        
        self.start_hour_entry =  ttk.Spinbox(start_time_frame, from_ = 0, to = 23,width = 4, wrap=True)
        self.start_hour_entry.pack(side=tk.LEFT)
        ttk.Label(start_time_frame, text=":").pack(side=tk.LEFT, padx=2)
        self.start_min_entry = ttk.Spinbox(start_time_frame, from_ = 0, to = 60,increment = 15,width = 4, wrap=True)
        self.start_min_entry.pack(side=tk.LEFT)

        # Stop time controls
        ttk.Label(time_grid, text="Stop Time:").grid(row=1, column=0, padx=(0, 5), pady=5, sticky="e")
        stop_time_frame = ttk.Frame(time_grid)
        stop_time_frame.grid(row=1, column=1, padx=(0, 15), pady=5, sticky="w")
        
        self.stop_hour_entry = ttk.Spinbox(stop_time_frame, from_ = 0, to = 23,width = 4, wrap=True)
        self.stop_hour_entry.pack(side=tk.LEFT)
        ttk.Label(stop_time_frame, text=":").pack(side=tk.LEFT, padx=2)
        self.stop_min_entry = ttk.Spinbox(stop_time_frame, from_ = 0, to = 60,increment = 15,width = 4, wrap=True)
        self.stop_min_entry.pack(side=tk.LEFT)

        # Time format hint
        time_hint_label = ttk.Label(
            time_frame,
            text="(Hours: 0-23, Minutes: 00, 15, 30, 45)",
            foreground="gray",
            font=("Arial", 8)
        )
        time_hint_label.pack(pady=(0, 5))

        # Force Time Sync and the skip-auto-sync diagnostic checkbox live in
        # the Bluetooth connection panel now (see _setup_bluetooth_controls)
        # - both are about the connection itself, not the schedule.

    def _setup_file_section(self, parent_frame):
        """Set up the file selection controls with better organization."""
        # Create labeled frame for file controls
        file_frame = ttk.LabelFrame(parent_frame, text="Audio File")
        file_frame.pack(side=tk.LEFT, fill="both", expand=True)

        # File controls in a grid
        file_grid = ttk.Frame(file_frame)
        file_grid.pack(pady=7, padx=10)

        # Folder selection
        ttk.Label(file_grid, text="Folder #:").grid(row=0, column=0, padx=(0, 5), pady=5, sticky="e")
        self.folder_entry = ttk.Entry(file_grid, width=8)
        self.folder_entry.grid(row=0, column=1, padx=(0, 15), pady=5, sticky="w")

        # File selection
        ttk.Label(file_grid, text="File #:").grid(row=1, column=0, padx=(0, 5), pady=5, sticky="e")
        self.file_entry = ttk.Entry(file_grid, width=8)
        self.file_entry.grid(row=1, column=1, padx=(0, 15), pady=5, sticky="w")

        # File format hint
        file_hint_label = ttk.Label(
            file_frame,
            text="(Folder and File numbers: 0-255)",
            foreground="gray",
            font=("Arial", 8)
        )
        file_hint_label.pack(pady=(0, 5))

    def _setup_action_buttons(self, parent_frame):
        """Set up the action buttons with better organization."""
        # Create labeled frame for action buttons
        action_frame = ttk.LabelFrame(parent_frame, text="Actions")
        action_frame.pack(fill="x")

        # Add entry button (primary action)
        self.add_entry_button = ttk.Button(
            action_frame,
            text="Add Entry",
            command=self.add_schedule_entry,
            style="Accent.TButton"
        )
        self.add_entry_button.pack(side=tk.LEFT, padx=10, pady=7, expand=True)

        # Schedule management buttons
        self.send_all_button = ttk.Button(
            action_frame,
            text="Send Schedules",
            command=self.send_all_schedules
        )
        self.send_all_button.pack(side=tk.LEFT, padx=5, pady=7, expand=True)

        # Clear queue button
        self.clear_queue_button = ttk.Button(
            action_frame,
            text="Clear Queue",
            command=self.clear_schedule_queue
        )
        self.clear_queue_button.pack(side=tk.LEFT, padx=5, pady=7, expand=True)

        # Export schedules button
        self.export_schedules_button = ttk.Button(
            action_frame,
            text="Export Schedules",
            command=self.export_schedules
        )
        self.export_schedules_button.pack(side=tk.LEFT, padx=5, pady=7, expand=True)

        # Import schedules button
        self.import_schedules_button = ttk.Button(
            action_frame,
            text="Import Schedules",
            command=self.import_schedules
        )
        self.import_schedules_button.pack(side=tk.LEFT, padx=5, pady=7, expand=True)

        # Clear schedule button - sends CLEARSCHEDULE (0x07) to the connected
        # device, matching the OLED's own "Clear Schedule" menu item. This is
        # distinct from "Clear Queue" above, which only clears the local
        # not-yet-sent queue in this GUI and never touches the device.
        self.clear_device_schedule_button = ttk.Button(
            action_frame,
            text="Clear Device Schedule",
            command=self.clear_device_schedule
        )
        self.clear_device_schedule_button.pack(side=tk.LEFT, padx=5, pady=7, expand=True)

    def _setup_log_section(self):
        """Set up the log and output section."""
        # Create frame for log controls. This column is narrower than the
        # Scheduler panel (which claims the flexible space), so the buttons
        # are two-per-row rather than relying on shrinking them to fit one
        # row - that stays reliable no matter how narrow this column ends up.
        bottom_button_frame = ttk.Frame(self.frame)
        bottom_button_frame.pack(fill="both", padx=10, pady=5)

        top_row = ttk.Frame(bottom_button_frame)
        top_row.pack(fill="x")

        bottom_row = ttk.Frame(bottom_button_frame)
        bottom_row.pack(fill="x", pady=(5, 0))

        # Download log button
        self.download_log_button = ttk.Button(
            top_row,
            text="Download Device Log",
            command=self.download_log
        )
        self.download_log_button.pack(side=tk.LEFT, padx=5, expand=True, fill="x")

        # Clear device log button - sends CLEARLOGS (0x08). Distinct from
        # both Clear Queue (local only) and Clear Device Schedule (schedule
        # flash region only) - this erases the stored bucket-summary/boot-
        # marker log entries so a Download Log afterward starts fresh
        # instead of returning old, potentially pre-reflash data.
        self.clear_device_log_button = ttk.Button(
            top_row,
            text="Clear Device Log",
            command=self.clear_device_log
        )
        self.clear_device_log_button.pack(side=tk.LEFT, padx=5, expand=True, fill="x")

        # Check status button
        self.check_status_button = ttk.Button(
            bottom_row,
            text="Check Device Status",
            command=self.check_status
        )
        self.check_status_button.pack(side=tk.LEFT, padx=5, expand=True, fill="x")

        # Check time button - sends TIMEREQUEST (0x10), a small read-only
        # diagnostic added to directly confirm what the device's RTC-derived
        # clock currently reads, instead of inferring it from log gaps.
        self.check_time_button = ttk.Button(
            bottom_row,
            text="Check Device Time",
            command=self.check_time
        )
        self.check_time_button.pack(side=tk.LEFT, padx=5, expand=True, fill="x")

        # Create text display area
        self._setup_text_display()

    def _setup_text_display(self):
        """Set up the text display area for logs and messages."""
        # Create frame for text display
        text_frame = ttk.Frame(self.frame)
        text_frame.pack(side=tk.LEFT, fill="both", expand=True, padx=(10, 0))

        # Create text widget with increased height
        self.devices_text = tk.Text(
            text_frame,
            height=30,  # Increased height
            wrap=tk.WORD,
            state=tk.DISABLED
        )
        self.devices_text.pack(side=tk.LEFT, fill="both", expand=True)

        # Add scrollbar
        scrollbar = ttk.Scrollbar(
            text_frame,
            command=self.devices_text.yview
        )
        scrollbar.pack(side=tk.RIGHT, fill="y")
        self.devices_text.config(yscrollcommand=scrollbar.set)

    def _setup_event_loop(self):
        """
        Set up the asyncio event loop for Bluetooth operations.
        
        This method initializes the event loop and starts it in a separate thread.
        It runs continuously until the application is closed.
        """
        # Initialize event loop
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        # Every coroutine that talks to the BLE GATT characteristics
        # (bluetooth_send, the log download, and the status check) must hold
        # this before touching self.ble_client. A single BLE connection can't
        # really have two GATT operations in flight at once - e.g. a Force
        # Time Sync click landing while a volume-slider "Set" is still
        # mid-transmission - and without serializing them here, their reads/
        # writes to the shared RX/TX/REQ_TX characteristics interleave on
        # the wire. That produces exactly the intermittent failures seen in
        # the field: "Failed to send data: disconnected" and "BLE client
        # not connected" errors that only show up when commands are fired
        # in quick succession, not when they're spaced out.
        self.ble_op_lock = asyncio.Lock()
        
        # Start event loop in separate thread
        self.loop_thread = threading.Thread(
            target=self._run_event_loop,
            daemon=True
        )
        self.loop_thread.start()

    def _init_connection_vars(self):
        """Initialize connection-related variables."""
        # Connection objects
        self.serial_conn = None
        self.ble_device = None
        self.ble_client = None
        
        # Connection settings
        self.ble_service_uuid = "d2de8bd0-2b7a-11f0-90a7-0800200c9a66"
        self.ble_tx_uuid = "d2de8bd1-2b7a-11f0-90a7-0800200c9a66"
        self.ble_rx_uuid = "d2de8bd2-2b7a-11f0-90a7-0800200c9a66"
        self.ble_req_tx_uuid = "d2de8bd3-2b7a-11f0-90a7-0800200c9a66"
        self.device_connected = False
        
        # Schedule management
        self.schedule_entries = []
        self.schedule_queue = []
        
        # Operation flags
        self.is_scanning = False
        self.scan_timeout = 10  # Increased scan timeout to 10 seconds
        self.debug_mode = True
        self.connection_retry_count = 0
        self.max_connection_retries = 3

        # Address of whichever discovered device is the current active
        # connection (or None) - drives the listbox highlight and the
        # connection-status label so it's always clear which of the
        # multiple visible speakers commands are actually going to.
        self.active_device_address = None
        # Guards against overlapping connect attempts (e.g. double-clicking
        # Connect) racing each other to set self.ble_client/self.ble_device.
        self._bt_connect_in_progress = False

        # Initialize button states based on connection type
        if self.connection_type.get() == "UART":
            # Enable UART controls
            self.baudrate_entry.config(state=tk.NORMAL)
            self.serial_listbox.config(state=tk.NORMAL)
            self.uart_connect_button.config(state=tk.NORMAL)
            self.refresh_button.config(state=tk.NORMAL)
            self.uart_disconnect_button.config(state=tk.DISABLED)

            # Disable Bluetooth controls
            self.devices_listbox.config(state=tk.DISABLED)
            self.bluetooth_connect_button.config(state=tk.DISABLED)
            self.scan_button.config(state=tk.DISABLED)
            self.bluetooth_disconnect_button.config(state=tk.DISABLED)
        else:
            # Enable Bluetooth controls
            self.devices_listbox.config(state=tk.NORMAL)
            self.bluetooth_connect_button.config(state=tk.NORMAL)
            self.scan_button.config(state=tk.NORMAL)
            self.bluetooth_disconnect_button.config(state=tk.DISABLED)

            # Disable UART controls
            self.baudrate_entry.config(state=tk.DISABLED)
            self.serial_listbox.config(state=tk.DISABLED)
            self.uart_connect_button.config(state=tk.DISABLED)
            self.refresh_button.config(state=tk.DISABLED)
            self.uart_disconnect_button.config(state=tk.DISABLED)

        # Disable all control buttons initially
        self.volume_set_button.config(state=tk.DISABLED)
        self.track_send_button.config(state=tk.DISABLED)
        self.duty_cycle_button.config(state=tk.DISABLED)
        self.add_entry_button.config(state=tk.DISABLED)
        self.send_all_button.config(state=tk.DISABLED)
        self.export_schedules_button.config(state=tk.DISABLED)
        self.import_schedules_button.config(state=tk.DISABLED)
        self.force_time_sync_button.config(state=tk.DISABLED)

    def _run_event_loop(self):
        """
        Run the asyncio event loop in a separate thread.
        
        This method is called by the event loop thread to handle all async operations.
        It runs continuously until the application is closed.
        """
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def run_async(self, coro):
        """
        Run a coroutine in the event loop from a non-async context.
        
        Args:
            coro: The coroutine to run
            
        Returns:
            The result of the coroutine
            
        Raises:
            Exception: If the operation times out or fails
        """
        try:
            future = asyncio.run_coroutine_threadsafe(coro, self.loop)
            return future.result(timeout=15)  # Increased timeout to 15 seconds
        except asyncio.TimeoutError:
            raise Exception("Operation timed out after 15 seconds")
        except Exception as e:
            raise Exception(f"Async operation failed: {str(e)}")

    def devices_text_insert(self, text, debug=False):
        """
        Insert text into the devices text display.
        
        Args:
            text (str): The text to insert
            debug (bool): If True, only show if debug_mode is enabled
        """
        if debug and not self.debug_mode:
            return  # Skip debug output unless debug_mode is on

        self.devices_text.config(state=tk.NORMAL)
        self.devices_text.insert(tk.END, text + "\n")
        self.devices_text.config(state=tk.DISABLED)
        self.devices_text.yview(tk.END)

    def clear_textbox(self):
        """Clear all text from the devices text display."""
        self.devices_text.config(state=tk.NORMAL)
        self.devices_text.delete("1.0", tk.END)
        self.devices_text.config(state=tk.DISABLED)

    def connect_to_device(self):
        """
        Connect to a device using the selected connection method.
        
        This method validates the connection configuration and attempts
        to establish a connection using either Bluetooth or UART.
        """
        if self.connection_type.get() == "Bluetooth":
            self.connect_to_bluetooth()
        elif self.connection_type.get() == "UART":
            if self.baudrate_var.get() and self.serial_listbox.curselection():
                self.connect_to_uart()
            else:
                self.devices_text_insert("Error: UART requires both baudrate and serial port.")
        else:
            self.devices_text_insert("Error: Invalid connection type selected.")

    def start_scan_devices(self):
        """
        Start scanning for Bluetooth devices.
        
        This method initiates a Bluetooth device scan if Bluetooth is selected
        as the connection type. It handles the scanning process in a separate
        thread to prevent UI freezing.
        """
        if self.connection_type.get() == "Bluetooth":
            if not self.is_scanning:
                self.devices_text_insert("[BT] Starting Bluetooth scan...", debug=True)
                self.is_scanning = True
                self.scan_button.config(state=tk.DISABLED)
                threading.Thread(target=self._run_scan, daemon=True).start()
        else:
            self.devices_text_insert("Error: Bluetooth not selected as desired connection method.")

    def _run_scan(self):
        """
        Run the Bluetooth device scan in a separate thread.
        
        This method handles the actual scanning process and updates the UI
        with the results. It runs in a separate thread to prevent UI freezing.
        """
        try:
            devices = self.run_async(self.scan_devices_async())
            self.after(0, lambda: self._update_scan_results(devices))
        except Exception as e:
            error_msg = str(e)  # Capture the error message
            if not error_msg:
                error_msg = "Unknown error during scan"
            self.after(0, lambda: self._handle_scan_error(error_msg))
        finally:
            self.after(0, self._finish_scan)

    def _filter_wps_devices(self, devices):
        """Return only devices that look like our Ambiance speakers (currently a no-op passthrough)."""
        filtered = []
        for d in devices:
            filtered.append(d)
        return filtered

    def _update_scan_results(self, devices):
        """
        Update the UI with the results of a Bluetooth device scan.
        
        Args:
            devices: List of discovered Bluetooth devices
        """
        # Store the discovered devices
        self.discovered_devices = devices
        
        self.devices_listbox.delete(0, tk.END)
        for device in devices:
            # Some devices may not have a name; fall back gracefully
            name = device.name or "Unknown"
            addr = getattr(device, "address", "??:??:??:??:??:??")

            # Show last 5 chars of address so they're easier to tell apart
            marker = "● " if addr == self.active_device_address else "  "
            label = f"{marker}{name} [{addr[-8:]}]"
            self.devices_listbox.insert(tk.END, label)

        self._refresh_device_listbox_highlight()
        self.devices_text_insert(f"[BT] Found {len(devices)} Ambiance speaker(s) during scan", debug=True)

    def _refresh_device_listbox_highlight(self):
        """
        Visually mark whichever entry in the device listbox is the active
        connection (self.active_device_address), so with multiple speakers
        visible at once it's always clear which one commands are being
        sent to. Safe to call any time the listbox or active device changes.
        """
        if not hasattr(self, "discovered_devices"):
            return
        # Read the listbox's own (theme/OS-provided) default colors rather
        # than hardcoding white/black - this widget is never given explicit
        # colors of its own, so it inherits the app's dark theme by default,
        # and hardcoding "white" here was overriding that for every row.
        default_bg = self.devices_listbox.cget("background")
        default_fg = self.devices_listbox.cget("foreground")
        for i, device in enumerate(self.discovered_devices):
            addr = getattr(device, "address", None)
            if self.active_device_address is not None and addr == self.active_device_address:
                self.devices_listbox.itemconfig(i, background="#2ecc71", foreground="black")
            else:
                self.devices_listbox.itemconfig(i, background=default_bg, foreground=default_fg)


    def _handle_scan_error(self, error_msg):
        """
        Handle errors that occur during Bluetooth device scanning.
        
        Args:
            error_msg (str): The error message to display
        """
        self.devices_text_insert(f"[BT][ERROR] Scan failed: {error_msg}")
        self.devices_text_insert("[BT] Please ensure Bluetooth is enabled and try again.", debug=True)

    def _finish_scan(self):
        """Clean up after a Bluetooth device scan is complete."""
        self.is_scanning = False
        self.scan_button.config(state=tk.NORMAL)

    async def scan_devices_async(self):
        """
        Asynchronously scan for Bluetooth devices.
        
        Returns:
            List of discovered Bluetooth devices
            
        Raises:
            Exception: If the scan fails
        """
        try:
            self.devices_text_insert("[BT] Starting BLE scan...", debug=True)
            devices = await BleakScanner.discover(timeout=self.scan_timeout)
            
            ambiance_devices = self._filter_wps_devices(devices)

            self.devices_text_insert(
            f"[BT] Found {len(devices)} devices, "
            f"{len(ambiance_devices)} Ambiance speaker devices",
            debug=True
            )

            return ambiance_devices
        except Exception as e:
            error_msg = str(e)
            if not error_msg:
                error_msg = "Unknown error during scan"
            raise Exception(f"Bluetooth scan failed: {error_msg}")

    def connect_to_bluetooth(self):
        """
        Connect to a selected Bluetooth device.

        This method initiates the connection process to the selected
        Bluetooth device. It handles the connection process in a separate
        thread to prevent UI freezing.
        """
        if self.connection_type.get() == "Bluetooth":
            if self._bt_connect_in_progress:
                self.devices_text_insert("[BT] A connection attempt is already in progress.", debug=True)
                return

            selection = self.devices_listbox.curselection()
            if not selection:
                self.devices_text_insert("[BT][ERROR] No Bluetooth device selected.")
                return

            index = selection[0]
            if not hasattr(self, "discovered_devices") or index >= len(self.discovered_devices):
                self.devices_text_insert("[BT][ERROR] Device list is out of date, please rescan and select again.")
                return

            # Resolve to a stable device address right now, synchronously,
            # instead of handing the background thread a listbox index - a
            # rescan landing before that thread runs could otherwise make
            # the index point at a different (or no longer existing) device.
            device_address = self.discovered_devices[index].address
            selected_device_name = self.devices_listbox.get(index)
            self.devices_text_insert(f"[BT] Attempting to connect to {selected_device_name}...", debug=True)

            # Disable connect button and guard against overlapping attempts
            self._bt_connect_in_progress = True
            self.bluetooth_connect_button.config(state=tk.DISABLED)

            # Start connection in a separate thread
            threading.Thread(target=self._run_bluetooth_connection, args=(device_address,), daemon=True).start()

    def _run_bluetooth_connection(self, device_address):
        """
        Run the Bluetooth connection process in a separate thread.

        Args:
            device_address (str): Bluetooth address of the device to connect to
        """
        try:
            self.run_async(self.async_connect_to_bluetooth(device_address))
        except Exception as e:
            error_msg = str(e)  # Capture the error message
            self.after(0, lambda: self.devices_text_insert(f"[BT][ERROR] Connection failed: {error_msg}"))
            self.after(0, lambda: self.update_connection_status(False, error_message="Connection failed"))
        finally:
            # Re-enable connect button
            self._bt_connect_in_progress = False
            self.after(0, lambda: self.bluetooth_connect_button.config(state=tk.NORMAL))

    async def async_connect_to_bluetooth(self, device_address: str):
        """
        Asynchronous Bluetooth connection logic.

        Args:
            device_address (str): Bluetooth address of the device to connect
                to. Resolved against self.discovered_devices at call time
                (not the listbox selection captured at click time), so a
                rescan landing in between can't connect to the wrong
                physical device - it either finds the same device again by
                address or reports it's no longer in range.
        """
        try:
            # Ensure we have a list of discovered devices
            if not hasattr(self, "discovered_devices") or not self.discovered_devices:
                # If no devices are stored, do a quick scan
                self.after(
                    0,
                    lambda: self.devices_text_insert(
                        "[BT] Scanning for devices...", debug=True
                    ),
                )
                devices = await BleakScanner.discover(timeout=self.scan_timeout)

                self.discovered_devices = devices
                self.after(
                    0,
                    lambda: self.devices_text_insert(
                        f"[BT] Found {len(self.discovered_devices)} devices during scan",
                        debug=True,
                    ),
                )

            # Resolve the address against the current device list.
            device = next(
                (d for d in self.discovered_devices if getattr(d, "address", None) == device_address),
                None,
            )

            if device is None:
                self.after(
                    0,
                    lambda: self.devices_text_insert(
                        f"[BT][ERROR] Device {device_address} is no longer in range; please rescan."
                    ),
                )
                self.after(
                    0,
                    lambda: self.update_connection_status(
                        False, error_message="Device not found"
                    ),
                )
                return

            # If we're already holding a connection to a (different) device,
            # tear it down first instead of silently orphaning it. Only one
            # BLE connection is meant to be active at a time, so switching
            # which speaker is selected should always leave exactly the new
            # one connected, not both (one leaked, unreachable).
            if self.ble_client is not None:
                self.after(
                    0,
                    lambda: self.devices_text_insert(
                        "[BT] Disconnecting from the previous device first...",
                        debug=True,
                    ),
                )
                await self._cleanup_connection()

            self.ble_device = device

            name = device.name or "Unknown"
            addr = getattr(device, "address", "unknown")
            label = f"{name} ({addr})"

            self.after(
                0,
                lambda: self.devices_text_insert(
                    f"[BT] Found device: {label}", debug=True
                ),
            )

            # Create client with timeout
            self.ble_client = BleakClient(device, timeout=30.0)

            # Attempt connection
            self.after(
                0,
                lambda: self.update_connection_status(
                    False, error_message="Connecting..."
                ),
            )
            self.after(
                0,
                lambda: self.devices_text_insert(
                    f"[BT] Connecting to {label}...", debug=True
                ),
            )

            try:
                await asyncio.wait_for(self.ble_client.connect(), timeout=30.0)
                self.after(
                    0,
                    lambda: self.devices_text_insert(
                        "[BT] Connected, verifying services...", debug=True
                    ),
                )
            except asyncio.TimeoutError:
                raise Exception("Connection attempt timed out after 30 seconds")
            except Exception as conn_error:
                raise Exception(f"Failed to establish connection: {str(conn_error)}")

            # Verify service and characteristics
            services = self.ble_client.services
            service_uuids = [s.uuid for s in services]

            if self.ble_service_uuid not in service_uuids:
                raise Exception("Device does not have required USART service")

            service = next(s for s in services if s.uuid == self.ble_service_uuid)
            characteristics = [c.uuid for c in service.characteristics]
            required_chars = [self.ble_tx_uuid, self.ble_rx_uuid, self.ble_req_tx_uuid]

            if not all(char in characteristics for char in required_chars):
                raise Exception(
                    "Device does not have all required USART characteristics - incompatible device"
                )

            # Connection successful
            self.device_connected = True
            self.connection_retry_count = 0  # Reset retry count on successful connection
            self.active_device_address = addr
            self.after(0, lambda: self.update_connection_status(True, "Bluetooth", device_label=label))
            self.after(0, self._refresh_device_listbox_highlight)
            self.after(
                0,
                lambda: self.devices_text_insert(
                    f"[BT] Connected successfully to {label}.", debug=True
                ),
            )
            return

        except asyncio.TimeoutError:
            # Handle scan timeout
            self.after(
                0,
                lambda: self.update_connection_status(
                    False, error_message="Connection timeout"
                ),
            )
            self.after(
                0,
                lambda: self.devices_text_insert(
                    "[BT][ERROR] Connection timed out. Please try again."
                ),
            )
        except Exception as e:
            # Handle other errors
            error_msg = str(e)
            self.after(
                0,
                lambda: self.update_connection_status(False, error_message=error_msg),
            )
            self.after(
                0,
                lambda: self.devices_text_insert(
                    f"[BT][ERROR] during connection: {error_msg}", debug=True
                ),
            )

            # Clean up on failure
            await self._cleanup_connection()

            # Check if this is a characteristic compatibility error (do not retry)
            if (
                "incompatible device" in error_msg.lower()
                or "required usart service" in error_msg.lower()
                or "required usart characteristics" in error_msg.lower()
                or "Device does not have required USART service" in error_msg
            ):
                self.after(
                    0,
                    lambda: self.devices_text_insert(
                        "[BT][ERROR] Device is incompatible - no retries will be attempted.",
                        debug=True,
                    ),
                )
                self.connection_retry_count = 0  # Reset retry count
            else:
                # Attempt retry if under max retries for other errors
                if self.connection_retry_count < self.max_connection_retries:
                    self.connection_retry_count += 1
                    self.after(1000, lambda: self._attempt_reconnect())
                else:
                    self.connection_retry_count = 0

    async def _cleanup_connection(self):
        """Clean up the Bluetooth connection."""
        if self.ble_client:
            try:
                if self.ble_client.is_connected:
                    await self.ble_client.disconnect()
            except Exception as e:
                # Always visible, not debug-only: if the physical
                # disconnect fails/times out here, the old connection can
                # remain alive at the OS/radio level even though we're
                # about to clear our own state and treat it as gone - that
                # mismatch is exactly what leaves connections stranded
                # (contending for the Mac's shared BLE connection slots)
                # across a multi-speaker session.
                self.after(0, lambda: self.devices_text_insert(
                    f"[BT][WARNING] Disconnect did not complete cleanly ({str(e)}) - "
                    f"the previous device's connection may still be live. If speakers "
                    f"start jittering/toggling or a new one won't connect, quit and "
                    f"restart the app (or toggle Bluetooth off/on) to clear it."
                ))
            finally:
                self.ble_client = None
                self.device_connected = False
                self.active_device_address = None
                self.after(0, self._refresh_device_listbox_highlight)

    def _run_bluetooth_send(self, data_bytes):
        """
        Run the Bluetooth send operation in a separate thread.

        Args:
            data_bytes (bytes): The data to send
        """
        try:
            self.run_async(self.bluetooth_send(data_bytes))
        except Exception as e:
            error_msg = str(e)
            self.after(0, lambda: self.devices_text_insert(f"[BT][ERROR] {error_msg}"))
            # Only retry for connection issues, not compatibility issues
            if ("not connected" in error_msg.lower() or "timeout" in error_msg.lower()) and not ("incompatible device" in error_msg.lower() or "required USART service" in error_msg.lower() or "required USART characteristics" in error_msg.lower() or "Device does not have required USART service" in error_msg):
                self.after(0, lambda: self.update_connection_status(False, error_message="Connection lost"))
                self.after(1000, self._attempt_reconnect)

    def send_over_bluetooth(self, data_bytes):
        """
        Send data over Bluetooth connection.
        
        Args:
            data_bytes (bytes): The data to send
        """
        if not self.device_connected:
            self.devices_text_insert("[BT][ERROR] Cannot send, no device connected.")
            return

        try:
            self.devices_text_insert(f"[BT][TX] Starting transmission: {list(data_bytes)}", debug=True)
            # Run the async operation in a separate thread
            threading.Thread(
                target=self._run_bluetooth_send,
                args=(data_bytes,),
                daemon=True
            ).start()
        except Exception as e:
            error_msg = str(e)
            self.devices_text_insert(f"[BT][ERROR] {error_msg}")
            # Only retry for connection issues, not compatibility issues
            if ("not connected" in error_msg.lower() or "timeout" in error_msg.lower()) and not ("incompatible device" in error_msg.lower() or "required USART service" in error_msg.lower() or "required USART characteristics" in error_msg.lower() or "Device does not have required USART service" in error_msg):
                self.update_connection_status(False, error_message="Connection lost")
                self.after(1000, self._attempt_reconnect)

    async def bluetooth_send(self, data_bytes):
        """
        Send data over Bluetooth, serialized against every other BLE GATT
        operation (see self.ble_op_lock) so an overlapping command - e.g. a
        Force Time Sync landing while a volume Set is still transmitting -
        can't interleave its RX/TX/REQ_TX traffic with this one. Concurrent,
        unserialized access to those characteristics was the actual cause of
        the intermittent "disconnected" / "BLE client not connected" errors
        seen when multiple controls were used in quick succession.
        """
        async with self.ble_op_lock:
            return await self._bluetooth_send_impl(data_bytes)

    async def _bluetooth_send_impl(self, data_bytes):
        """
        Asynchronously send data over Bluetooth following the microcontroller protocol:
        1. Write data to RX
        2. Write 1 to TX_REQ to request transmission
        3. Continuously read from TX buffer one byte at a time until TX_REQ becomes 2
        4. Write 1 to TX_REQ after each read to acknowledge receipt
        """
        if not self.ble_client or not self.ble_client.is_connected:
            raise Exception("BLE client not connected")

        try:
            # Create a buffer for messages
            message_buffer = []
            response_bytes = []
            
            # Write data to USART_RX characteristic
            for byte in data_bytes:
                byte = byte.to_bytes(1,'big')
                await self.ble_client.write_gatt_char(self.ble_rx_uuid, byte)
                # Bind the *current* byte as a default arg, not a closure over the
                # loop variable - self.after() only queues this for Tkinter to run
                # later, and by then the loop may already be on a later byte, so a
                # bare "lambda: ...byte..." would print whatever byte the loop had
                # reached by the time Tkinter got around to it, not the one that was
                # actually just written. This was purely a misleading log message -
                # the actual write_gatt_char call above always sent the right byte -
                # but it looked exactly like real data corruption (e.g. two "[15]"
                # prints in a row for an intended [0, 15]) when it wasn't.
                self.after(0, lambda b=byte: self.devices_text_insert(f"[BT][RX] Data written to RX: {list(b)}", debug=True))
                await asyncio.sleep(0.01)  # 10ms delay
            
            
            # Write 1 to USART_REQ_TX to request transmission
            await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))
            self.after(0, lambda: self.devices_text_insert("[BT][TX] Transmission requested", debug=True))
            
            # Add a small delay after requesting transmission
            await asyncio.sleep(0.1)  # 100ms delay
            
            # Poll for response
            max_attempts = 80  # Maximum number of polling attempts
            attempt = 0
            last_tx_req = None
            transmission_confirmed = False
            
            while attempt < max_attempts:
                # First read from TX to empty the buffer
                try:
                    debug_msg = await self.ble_client.read_gatt_char(self.ble_tx_uuid)
                    if debug_msg:
                        # Add byte to response buffer
                        response_bytes.extend(debug_msg)
                        # Write 1 to TX_REQ to acknowledge receipt
                        await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))
                        self.after(0, lambda: self.devices_text_insert("[BT][TX_REQ] Acknowledged receipt with 1", debug=True))
                except Exception as e:
                    # Log read errors for debugging (bind attempt as a default arg -
                    # same late-binding issue as the RX byte print above)
                    self.after(0, lambda a=attempt: self.devices_text_insert(f"[BT][TX] No data available on read attempt {a}", debug=True))
                
                # Then check TX_REQ status
                try:
                    tx_req = await self.ble_client.read_gatt_char(self.ble_req_tx_uuid)
                    req_value = tx_req[0]
                    
                    if last_tx_req != req_value:
                        # Bind both values now, as default args - otherwise this
                        # deferred print can end up showing the *post*-assignment
                        # last_tx_req (since it's reassigned on the very next line),
                        # producing nonsensical output like "changed from 2 to 2"
                        # even though a real transition happened.
                        self.after(0, lambda old=last_tx_req, new=req_value: self.devices_text_insert(f"[BT][TX_REQ] State changed from {old} to {new}", debug=True))
                        last_tx_req = req_value
                    
                    if req_value == 2:
                        # Transmission complete
                        transmission_confirmed = True
                        self.after(0, lambda: self.devices_text_insert("[BT][TX_REQ] Transmission complete signal received", debug=True))
                        break
                except Exception as e:
                    self.after(0, lambda: self.devices_text_insert(f"[BT][TX_REQ] Error reading state: {str(e)}", debug=True))
                
                # Wait a short time before next poll
                await asyncio.sleep(0.01)  # 10ms delay between polls
                attempt += 1
                
                if attempt % 5 == 0:
                    self.after(0, lambda a=attempt: self.devices_text_insert(f"[BT] Polling attempt {a}/{max_attempts}", debug=True))
            
            # Process final results
            if response_bytes:
                # Try to decode the complete response
                try:
                    # Try UTF-8 first
                    message = bytes(response_bytes).decode('utf-8', errors='replace').strip()
                    if message:
                        message_buffer.append(f"Complete message (UTF-8): {message}")
                except Exception:
                    pass
                
                try:
                    # Try ASCII as fallback
                    message = bytes(response_bytes).decode('ascii', errors='replace').strip()
                    if message:
                        message_buffer.append(f"Complete message (ASCII): {message}")
                except Exception:
                    pass
                
                if message_buffer:
                    combined_message = "\n".join(message_buffer)
                    self.after(0, lambda: self.devices_text_insert(f"[BT][TX] Received message:\n{combined_message}", debug=True))
                else:
                    self.after(0, lambda: self.devices_text_insert("[BT][TX] No valid message received", debug=True))
            else:
                self.after(0, lambda: self.devices_text_insert("[BT][TX] No messages received after all attempts", debug=True))

            if not transmission_confirmed:
                # The loop exhausted max_attempts without ever seeing
                # TX_REQ==2 - the device never confirmed it received/
                # processed the data. This used to be treated the same as
                # success (a misleading "Communication complete" was always
                # printed), so a dropped write - e.g. a time sync during
                # connection contention - looked identical to it working.
                raise Exception("Transmission not confirmed by device (timed out waiting for TX_REQ=2)")

            self.after(0, lambda: self.devices_text_insert("[BT][TX] Communication complete", debug=True))
        except Exception as e:
            raise Exception(f"Failed to send data: {str(e)}")

    def _attempt_reconnect(self):
        """Attempt to reconnect to the Bluetooth device."""
        if self.ble_device and not self.device_connected:
            self.devices_text_insert("[BT] Attempting to reconnect...", debug=True)
            # _run_bluetooth_connection/async_connect_to_bluetooth take a
            # device *address* (str), not a name - this used to pass
            # self.ble_device.name, which raised TypeError the moment
            # async_connect_to_bluetooth compared it against an int and got
            # silently swallowed by the caller's broad except, so automatic
            # reconnect never actually worked.
            threading.Thread(target=self._run_bluetooth_connection, args=(self.ble_device.address,), daemon=True).start()

    def uart_button_toggled(self):
        """
        Handle changes in the connection type selection.
        
        This method updates the UI state based on whether Bluetooth
        or UART is selected as the connection type.
        """
        # Disconnect from current device if connected
        if self.device_connected:
            self.disconnect_device()

        if self.connection_type.get() == "UART":
            # Enable UART controls
            self.baudrate_entry.config(state=tk.NORMAL)
            self.serial_listbox.config(state=tk.NORMAL)
            self.uart_connect_button.config(state=tk.NORMAL)
            self.refresh_button.config(state=tk.NORMAL)
            self.uart_disconnect_button.config(state=tk.DISABLED)

            # Disable Bluetooth controls
            self.devices_listbox.config(state=tk.DISABLED)
            self.bluetooth_connect_button.config(state=tk.DISABLED)
            self.scan_button.config(state=tk.DISABLED)
            self.bluetooth_disconnect_button.config(state=tk.DISABLED)

            # Update connection status
            self.update_connection_status(False, "UART (Not Connected)")

        else:
            # Disable UART controls
            self.baudrate_entry.config(state=tk.DISABLED)
            self.serial_listbox.config(state=tk.DISABLED)
            self.uart_connect_button.config(state=tk.DISABLED)
            self.refresh_button.config(state=tk.DISABLED)
            self.uart_disconnect_button.config(state=tk.DISABLED)

            # Enable Bluetooth controls
            self.devices_listbox.config(state=tk.NORMAL)
            self.bluetooth_connect_button.config(state=tk.NORMAL)
            self.scan_button.config(state=tk.NORMAL)

            # Update connection status
            self.update_connection_status(False, "Bluetooth (Not Connected)")

    def populate_serial_ports(self):
        """
        Populate the list of available serial ports.
        
        This method scans the system for available serial ports and
        updates the serial port listbox with the results.
        """
        self.serial_ports = list(serial.tools.list_ports.comports())
        self.serial_listbox.delete(0, tk.END)
        for port in self.serial_ports:
            self.serial_listbox.insert(tk.END, port.device)

    def refresh_serial_ports(self):
        """
        Refresh the list of available serial ports.
        
        This method updates the serial port list when the refresh
        button is clicked. It only works when UART is selected.
        """
        if self.connection_type.get() == "UART":
            self.populate_serial_ports()
        else:
            self.devices_text_insert("Error: UART must be selected to refresh serial ports.")

    def ensure_device_connected(self):
        """
        Check if a device is connected before executing commands.
        
        Returns:
            bool: True if a device is connected, False otherwise
        """
        if not self.device_connected:
            self.devices_text_insert("Error: No device connected. Connect via Bluetooth or UART first.")
            return False
        return True
    
    def poll_uart_data(self):
        """
        Continuously check for incoming UART data.
        
        This method polls the serial connection for incoming data
        and displays it in the text window. It schedules itself to
        run again after a short delay, but only if the connection is active.
        """
        # Only continue polling if UART is connected and device is connected
        if (self.connection_type.get() == "UART" and
            self.serial_conn and
            self.device_connected and
            self.serial_conn.is_open):
            
            if self.serial_conn.in_waiting:
                try:
                    data = self.serial_conn.readline().decode('utf-8', errors='replace').strip()
                    if data:
                        self.devices_text_insert(f"[UART][RX] {data}", debug=True)
                except Exception as e:
                    self.devices_text_insert(f"[UART][ERROR] {e}", debug=True)
            
            # Schedule next poll only if still connected
            self.after(200, self.poll_uart_data)
        else:
            # Stop polling if connection is lost
            self.devices_text_insert("[UART] Polling stopped - connection lost", debug=True)

    def _connected_device_short_name(self):
        """
        Best-effort 4-character speaker identifier for the connected device,
        matching what's shown in the scan list and on the OLED (e.g.
        "Ambiance Speaker 2CE5" -> "2CE5"). Falls back to "UART" when
        connected over a serial cable (no advertised name to draw from), or
        "device" if a Bluetooth name doesn't look like the expected format.
        """
        if self.connection_type.get() == "UART":
            return "UART"
        name = getattr(self.ble_device, "name", None) if self.ble_device else None
        if name:
            last_token = name.strip().split()[-1]
            if last_token:
                return last_token
        return "device"

    def download_log(self):
        """Request and download system log from the device (UART or Bluetooth)."""
        if not self.ensure_device_connected():
            self.devices_text_insert("Error: No device connected.")
            return

        # Ask where to save before contacting the device, same as Export Schedules.
        # Date only (no time-of-day) - a same-day re-download just prompts the
        # normal save-dialog overwrite confirmation rather than silently
        # piling up timestamped files.
        date_str = datetime.now().strftime("%Y%m%d")
        short_name = self._connected_device_short_name()
        file_path = filedialog.asksaveasfilename(
            title="Save Log As",
            defaultextension=".txt",
            filetypes=[("Text Files", "*.txt")],
            initialfile=f"log_{date_str}_{short_name}.txt"
        )
        if not file_path:
            self.devices_text_insert("Download canceled by user.")
            return

        self.devices_text_insert("Requesting log download...")

        if self.connection_type.get() == "UART" and self.serial_conn:
            try:
                # UART Download Logic
                self.devices_text_insert("[UART][TX] Sending log request command: 0x02", debug=True)
                self.serial_conn.write(bytes([0x02]))

                high = self.serial_conn.read(1)
                low = self.serial_conn.read(1)

                if not high or not low:
                    self.devices_text_insert("[UART][ERROR] Failed to receive log size.")
                    return

                entry_count = (high[0] << 8) | low[0]
                size = entry_count * self.LOG_ENTRY_SIZE
                self.devices_text_insert(f"[UART][RX] Log size received: {entry_count} entries ({size} bytes)", debug=True)

                received_data = b""
                while len(received_data) < size:
                    chunk = self.serial_conn.read(size - len(received_data))
                    if not chunk:
                        break
                    received_data += chunk
                    self.devices_text_insert(f"[UART][RX] Received {len(received_data)} / {size} bytes...", debug=True)

                log_text = self.format_log_entries(received_data)
                self.save_log(log_text, file_path)

            except Exception as e:
                self.devices_text_insert(f"[UART][ERROR] during log download: {e}", debug=True)

        elif self.connection_type.get() == "Bluetooth" and self.device_connected:
            try:
                # Bluetooth Download Logic
                async def ble_download():
                    # Serialize against bluetooth_send()/ble_status() - see
                    # self.ble_op_lock's definition. A log download in
                    # progress must not interleave its RX/TX/REQ_TX traffic
                    # with e.g. a schedule send fired mid-download.
                    async with self.ble_op_lock:
                        await ble_download_impl()

                async def ble_download_impl():
                    if self.ble_client and self.ble_client.is_connected:
                        # Send the request command directly here - do NOT route
                        # this through send_over_bluetooth()/bluetooth_send(). That
                        # helper runs its own independent read/ack polling loop on
                        # these same characteristics in a separate task, and it
                        # will race with (and consume bytes meant for) the reads
                        # below instead of leaving them for this exchange.
                        self.devices_text_insert("[BT][TX] Sending log request command: 0x02", debug=True)
                        await self.ble_client.write_gatt_char(self.ble_rx_uuid, bytes([0x02]))

                        # Protocol: a byte only lands in the TX characteristic in
                        # response to a write on REQ_TX - but since the firmware side
                        # was widened to drain up to ~20 queued bytes per REQ_TX
                        # request instead of exactly 1 (for BLE transfer speed), a
                        # single request here can now return the 2 size bytes AND
                        # the start of the actual log entries all in the same
                        # response. The old code assumed "high" and "low" were each
                        # exactly one byte and threw away anything past index 0 -
                        # that silently discarded real log data on every download
                        # once the firmware started batching, which is exactly what
                        # produced garbled/shifted-looking log entries (nonsense
                        # months/days) even though nothing was wrong with the RTC or
                        # the scheduler. Fix: accumulate everything received into one
                        # running buffer and slice fields off the front of it, rather
                        # than assuming a chunk boundary lines up with a field
                        # boundary.
                        raw = b""

                        async def request_more():
                            nonlocal raw
                            await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))
                            chunk = await self.ble_client.read_gatt_char(self.ble_tx_uuid)
                            if chunk:
                                raw += chunk
                            return chunk

                        while len(raw) < 2:
                            if not await request_more():
                                self.devices_text_insert("[BT][ERROR] Failed to receive log size.")
                                return

                        entry_count = (raw[0] << 8) | raw[1]
                        size = entry_count * self.LOG_ENTRY_SIZE
                        self.devices_text_insert(f"[BT][RX] Log size received: {entry_count} entries ({size} bytes)", debug=True)

                        # Report progress periodically rather than on every byte/chunk -
                        # printing + scheduling a Tkinter UI update per read is real
                        # avoidable overhead on top of the BLE round trips themselves.
                        last_reported = 0
                        while (len(raw) - 2) < size:
                            n = max(len(raw) - 2, 0)
                            if n - last_reported >= 50:
                                last_reported = n
                                self.devices_text_insert(f"[BT][RX] Received {n} / {size} bytes...", debug=True)
                            if not await request_more():
                                break

                        received_data = raw[2:2 + size]
                        n = len(received_data)
                        self.devices_text_insert(f"[BT][RX] Received {n} / {size} bytes...", debug=True)

                        log_text = self.format_log_entries(received_data)
                        self.after(0, lambda: self.save_log(log_text, file_path))
                    else:
                        self.devices_text_insert("[BT][ERROR] No BLE connection active.")

                def _run_log_download():
                    try:
                        future = asyncio.run_coroutine_threadsafe(ble_download(), self.loop)
                        future.result()  # no short timeout: log downloads can take a while
                    except Exception as e:
                        error_msg = str(e)
                        self.after(0, lambda: self.devices_text_insert(f"[BT][ERROR] during log download: {error_msg}", debug=True))

                threading.Thread(target=_run_log_download, daemon=True).start()

            except Exception as e:
                self.devices_text_insert(f"[BT][ERROR] during log download: {e}", debug=True)

        else:
            self.devices_text_insert("Error: Log download only supported over UART or Bluetooth.")

    STATUS_LABELS = {
        0: "Not broadcasting (device not responding / not initialized)",
        1: "Broadcasting",
        2: "Programmed silence",
    }

    def check_status(self):
        """Query whether the speaker is currently broadcasting, programmed-silent, or dead."""
        if not self.ensure_device_connected():
            self.devices_text_insert("Error: No device connected.")
            return

        self.devices_text_insert("Requesting status...")

        if self.connection_type.get() == "UART" and self.serial_conn:
            try:
                self.devices_text_insert("[UART][TX] Sending status request command: 0x06", debug=True)
                self.serial_conn.write(bytes([0x06]))

                reply = self.serial_conn.read(3)
                if len(reply) < 3:
                    self.devices_text_insert("[UART][ERROR] No response to status request.")
                    return

                self._show_status(reply[0], reply[1], reply[2])

            except Exception as e:
                self.devices_text_insert(f"[UART][ERROR] during status request: {e}", debug=True)

        elif self.connection_type.get() == "Bluetooth" and self.device_connected:
            try:
                self.devices_text_insert("[BT][TX] Sending status request command: 0x06", debug=True)

                async def ble_status():
                    # Serialize against bluetooth_send()/ble_download() - see
                    # self.ble_op_lock's definition.
                    async with self.ble_op_lock:
                        await ble_status_impl()

                async def ble_status_impl():
                    if not (self.ble_client and self.ble_client.is_connected):
                        self.devices_text_insert("[BT][ERROR] No BLE connection active.")
                        return

                    await self.ble_client.write_gatt_char(self.ble_rx_uuid, bytes([0x06]))
                    await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))

                    reply = bytearray()
                    for _ in range(50):  # up to ~1s at 20ms/poll
                        await asyncio.sleep(0.02)
                        chunk = await self.ble_client.read_gatt_char(self.ble_tx_uuid)
                        if chunk:
                            reply.extend(chunk)
                            await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))
                            if len(reply) >= 3:
                                break

                    if len(reply) < 3:
                        self.after(0, lambda: self.devices_text_insert("[BT][ERROR] No response to status request."))
                    else:
                        status_byte, folder_byte, track_byte = reply[0], reply[1], reply[2]
                        self.after(0, lambda: self._show_status(status_byte, folder_byte, track_byte))

                def _run_status_check():
                    try:
                        future = asyncio.run_coroutine_threadsafe(ble_status(), self.loop)
                        future.result(timeout=15)
                    except Exception as e:
                        error_msg = str(e)
                        self.after(0, lambda: self.devices_text_insert(f"[BT][ERROR] during status request: {error_msg}", debug=True))

                threading.Thread(target=_run_status_check, daemon=True).start()

            except Exception as e:
                self.devices_text_insert(f"[BT][ERROR] during status request: {e}", debug=True)

        else:
            self.devices_text_insert("Error: Status check only supported over UART or Bluetooth.")

    def _show_status(self, status_byte, folder, track):
        """Display the decoded status/folder/track reply from check_status()."""
        label = self.STATUS_LABELS.get(status_byte, f"Unknown status byte: {status_byte}")

        if status_byte == 1:
            text = f"{label}  (Folder {folder} Track {track})"
        elif status_byte == 2:
            text = f"{label}  (last played Folder {folder} Track {track})"
        else:
            text = label

        self.devices_text_insert(f"Speaker status: {text}")

    def check_time(self):
        """
        Query the device's current RTC-derived month/day/hour/minute
        (TIMEREQUEST, 0x10). A read-only diagnostic - lets you directly
        confirm the device's clock is valid and advancing, instead of
        inferring it from gaps in a downloaded log.
        """
        if not self.ensure_device_connected():
            self.devices_text_insert("Error: No device connected.")
            return

        self.devices_text_insert("Requesting device time...")

        if self.connection_type.get() == "UART" and self.serial_conn:
            try:
                self.devices_text_insert("[UART][TX] Sending time request command: 0x10", debug=True)
                self.serial_conn.write(bytes([0x10]))

                reply = self.serial_conn.read(4)
                if len(reply) < 4:
                    self.devices_text_insert("[UART][ERROR] No response to time request.")
                    return

                self._show_time(reply[0], reply[1], reply[2], reply[3])

            except Exception as e:
                self.devices_text_insert(f"[UART][ERROR] during time request: {e}", debug=True)

        elif self.connection_type.get() == "Bluetooth" and self.device_connected:
            try:
                self.devices_text_insert("[BT][TX] Sending time request command: 0x10", debug=True)

                async def ble_time():
                    # Serialize against bluetooth_send()/ble_download()/ble_status() -
                    # see self.ble_op_lock's definition.
                    async with self.ble_op_lock:
                        await ble_time_impl()

                async def ble_time_impl():
                    if not (self.ble_client and self.ble_client.is_connected):
                        self.devices_text_insert("[BT][ERROR] No BLE connection active.")
                        return

                    await self.ble_client.write_gatt_char(self.ble_rx_uuid, bytes([0x10]))
                    await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))

                    reply = bytearray()
                    for _ in range(50):  # up to ~1s at 20ms/poll
                        await asyncio.sleep(0.02)
                        chunk = await self.ble_client.read_gatt_char(self.ble_tx_uuid)
                        if chunk:
                            reply.extend(chunk)
                            await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))
                            if len(reply) >= 4:
                                break

                    if len(reply) < 4:
                        self.after(0, lambda: self.devices_text_insert("[BT][ERROR] No response to time request."))
                    else:
                        month_byte, day_byte, hour_byte, minute_byte = reply[0], reply[1], reply[2], reply[3]
                        self.after(0, lambda: self._show_time(month_byte, day_byte, hour_byte, minute_byte))

                def _run_time_check():
                    try:
                        future = asyncio.run_coroutine_threadsafe(ble_time(), self.loop)
                        future.result(timeout=15)
                    except Exception as e:
                        error_msg = str(e)
                        self.after(0, lambda: self.devices_text_insert(f"[BT][ERROR] during time request: {error_msg}", debug=True))

                threading.Thread(target=_run_time_check, daemon=True).start()

            except Exception as e:
                self.devices_text_insert(f"[BT][ERROR] during time request: {e}", debug=True)

        else:
            self.devices_text_insert("Error: Time check only supported over UART or Bluetooth.")

    def _show_time(self, month, day, hour, minute):
        """Display the decoded month/day/hour/minute reply from check_time()."""
        if month == 0 or day == 0:
            self.devices_text_insert(
                f"Device time: Month {month:02d} Day {day:02d} {hour:02d}:{minute:02d}  "
                "(not yet valid - device hasn't seen a real time sync since its last reset)"
            )
        else:
            self.devices_text_insert(f"Device time: Month {month:02d} Day {day:02d} {hour:02d}:{minute:02d}")

    LOG_ENTRY_SIZE = 7  # month, daystart, start, daystop, stop, folder, track
    LOG_BUCKET_HOURS = 2  # must match LOGBUCKETHOURS in Scheduler.c
    LOG_BOOT_MARKER = 2  # stop==2 marks a boot-reset record instead of a bucket summary (see Scheduler.c)
    LOG_PROGRAMMED_SILENCE = 3  # bucket summary stop==3: MP3 confirmed alive/idle by design, never actually broadcast (see Scheduler.c)

    # Must match RESETCAUSE_* in Scheduler.h
    RESET_CAUSE_BITS = [
        (1 << 0, "power-on/brown-out"),
        (1 << 1, "external reset pin"),
        (1 << 2, "software reset"),
        (1 << 3, "watchdog"),
        (1 << 4, "CPU lockup"),
    ]

    def _decode_reset_cause(self, cause_byte):
        """Turn a RESETCAUSE_* bitmask byte into a human-readable list of causes."""
        causes = [label for bit, label in self.RESET_CAUSE_BITS if cause_byte & bit]
        if not causes:
            return f"unknown (0x{cause_byte:02X})"
        return ", ".join(causes)

    def format_log_entries(self, data):
        """
        Decode raw log bytes from the device into a human-readable log.

        Each entry is either a bucket summary or a boot marker, both packed
        into the same 7-byte scheduleEvent wire format: month, daystart,
        start, daystop, stop, folder, track.

        Bucket summary (stop is 0, 1, or 3): covers one LOG_BUCKET_HOURS-wide
        window of a calendar day. `start` is which bucket this entry covers
        (0 = 00:00, 1 = 02:00, ...). `stop` is 1 if the speaker broadcast at
        least once during that window; 3 if it never broadcast but was
        confirmed alive and idle by design the whole window (mid duty-cycle
        pause, or simply outside any scheduled window - "programmed
        silence", not a failure); 0 if neither was ever observed, meaning
        the device never responded at all during that window ("dead"/
        unresponsive). `folder`/`track` are the last track played in that
        window (0 if it never played). `daystop` is unused.

        Boot marker (stop == LOG_BOOT_MARKER): logged once per device boot,
        the first time RTC time becomes valid again. `start` holds the boot
        time-of-day packed as hour<<3 | (minute//15), same scheme as a
        schedule entry's start/stop times. `folder` holds a RESETCAUSE_*
        bitmask (see Scheduler.h) saying why the device reset - in
        particular, "power-on/brown-out" is the signature of a power-supply
        or charging-circuit brownout rather than a firmware fault. `track`
        and `daystop` are unused.
        """
        lines = []
        entry_count = len(data) // self.LOG_ENTRY_SIZE
        for i in range(entry_count):
            offset = i * self.LOG_ENTRY_SIZE
            month, daystart, packed, daystop, played, folder, track = data[offset:offset + self.LOG_ENTRY_SIZE]

            if played == self.LOG_BOOT_MARKER:
                boot_hour = (packed & 0b11111000) >> 3
                boot_min = (packed & 0b00000011) * 15
                cause_text = self._decode_reset_cause(folder)
                lines.append(
                    f"Month {month:02d} Day {daystart:02d} {boot_hour:02d}:{boot_min:02d}: "
                    f"Device booted (cause: {cause_text})"
                )
                continue

            bucket_start_hour = (packed * self.LOG_BUCKET_HOURS) % 24
            bucket_end_hour = bucket_start_hour + self.LOG_BUCKET_HOURS
            window = f"{bucket_start_hour:02d}:00-{bucket_end_hour:02d}:00"

            if played == 1:
                lines.append(f"Month {month:02d} Day {daystart:02d} {window}: Broadcast  (last played Folder {folder} Track {track})")
            elif played == self.LOG_PROGRAMMED_SILENCE:
                lines.append(f"Month {month:02d} Day {daystart:02d} {window}: Programmed silence (device alive, idle by design)")
            else:
                lines.append(f"Month {month:02d} Day {daystart:02d} {window}: No broadcast (device unresponsive)")

        leftover = len(data) % self.LOG_ENTRY_SIZE
        if leftover:
            lines.append(f"[WARNING] {leftover} trailing byte(s) did not form a complete log entry")

        return "\n".join(lines) if lines else "(no log entries)"

    def save_log(self, log_text, file_path):
        """Preview the downloaded log and write it to the path chosen at the start of download_log()."""
        preview = log_text[:300] + ("..." if len(log_text) > 300 else "")
        self.devices_text_insert("Log Preview:\n" + preview)

        with open(file_path, "w", encoding="utf-8") as f:
            f.write(log_text)
        self.devices_text_insert(f"Log saved to {file_path}")

    def add_schedule_entry(self):
        """Validate and add a schedule entry to the queue (but don't send it)."""
        try:
            month = int(self.month_entry.get())
            start_day = int(self.start_day_entry.get())
            end_day = int(self.end_day_entry.get())
            start_hour = int(self.start_hour_entry.get())
            start_min = int(self.start_min_entry.get())
            stop_hour = int(self.stop_hour_entry.get())
            stop_min = int(self.stop_min_entry.get())
            folder = int(self.folder_entry.get())
            file = int(self.file_entry.get())

            valid_minutes = [0, 15, 30, 45]
            if start_min not in valid_minutes or stop_min not in valid_minutes:
                self.devices_text_insert("Error: Minutes must be 00, 15, 30, or 45.")
                return

            if (stop_hour, stop_min) <= (start_hour, start_min):
                self.devices_text_insert("Error: Stop time must be after start time.")
                return

            if start_day > end_day:
                self.devices_text_insert("Error: Start day must be before or equal to end day.")
                return

            if not (1 <= start_day <= 31) or not (1 <= end_day <= 31):
                self.devices_text_insert("Error: Days must be between 1 and 31.")
                return

            # Check for overlap with existing schedules
            for entry in self.schedule_queue:
                if entry["month"] == month:
                    # Check if day ranges overlap
                    if not (end_day < entry["start_day"] or start_day > entry["end_day"]):
                        # Day ranges overlap, check if time ranges also overlap
                        existing_start = (entry["start_hour"], entry["start_min"])
                        existing_stop = (entry["stop_hour"], entry["stop_min"])
                        new_start = (start_hour, start_min)
                        new_stop = (stop_hour, stop_min)
                        
                        # Check if time ranges overlap
                        if not (new_stop <= existing_start or new_start >= existing_stop):
                            self.devices_text_insert("Error: Overlap with an existing queued schedule.")
                            return

            new_entry = {
                "month": month,
                "start_day": start_day,
                "end_day": end_day,
                "start_hour": start_hour,
                "start_min": start_min,
                "stop_hour": stop_hour,
                "stop_min": stop_min,
                "folder": folder,
                "file": file
            }

            self.schedule_queue.append(new_entry)
            self.devices_text_insert(
                f"[Queued] {month:02d}/{start_day:02d}-{end_day:02d} | {start_hour:02d}:{start_min:02d} - "
                f"{stop_hour:02d}:{stop_min:02d} | Folder #{folder}, File #{file}"
            )

        except ValueError:
            self.devices_text_insert("Error: Fill all scheduler fields with valid numbers.")

    def send_all_schedules(self):
        """Send all queued schedules to the device (UART or Bluetooth)."""
        if not self.ensure_device_connected():
            self.devices_text_insert("Error: No device connected.")
            return

        if not self.schedule_queue:
            self.devices_text_insert("Error: No schedules queued.")
            return

        try:
            # Command bytes
            start_batch = bytes([0x05])  # Start schedule transmission
            end_batch = bytes([0x0D])    # End schedule transmission

            # Function to encode time (hour and minute) into a single byte
            def encode_time(h, m):
                return ((h & 0b11111) << 3) | (m // 15)  # 5 bits for hour, 3 bits for 15-min intervals

            if self.connection_type.get() == "UART" and self.serial_conn:
                # UART sending
                self.devices_text_insert("[UART][TX] Sending start batch command (0x05)", debug=True)
                tosend = start_batch
                #self.serial_conn.write(start_batch)

                for sched in self.schedule_queue:
                    # New protocol format: [month, start_day, start_time, end_day, end_time, folder, track]
                    encoded_schedule = bytes([
                        sched["month"],
                        sched["start_day"],
                        encode_time(sched["start_hour"], sched["start_min"]),
                        sched["end_day"],
                        encode_time(sched["stop_hour"], sched["stop_min"]),
                        sched["folder"],  # folder first
                        sched["file"]     # track second
                    ])
                    tosend += encoded_schedule
                    #self.serial_conn.write(encoded_schedule)
                    self.devices_text_insert(f"[UART][TX] Sent schedule: {encoded_schedule}", debug=True)
                tosend += end_batch
                self.serial_conn.write(tosend)
                self.devices_text_insert("[UART][TX] Sent end batch command (0x0D)", debug=True)

                self.devices_text_insert(f"[UART] Sent {len(self.schedule_queue)} schedule(s) to device.")
                self.schedule_queue.clear()

            elif self.connection_type.get() == "Bluetooth" and self.device_connected:
                # Bluetooth sending
                self.devices_text_insert("[BT][TX] Sending start batch command (0x05)", debug=True)
                tosend = start_batch
                #self.send_over_bluetooth(start_batch)

                for sched in self.schedule_queue:
                    # New protocol format: [month, start_day, start_time, end_day, end_time, folder, track]
                    encoded_schedule = bytes([
                        sched["month"],
                        sched["start_day"],
                        encode_time(sched["start_hour"], sched["start_min"]),
                        sched["end_day"],
                        encode_time(sched["stop_hour"], sched["stop_min"]),
                        sched["folder"],  # folder first
                        sched["file"]     # track second
                    ])
                    tosend += encoded_schedule
                    #self.send_over_bluetooth(encoded_schedule)
                    self.devices_text_insert(f"[BT][TX] Sent schedule: {encoded_schedule}", debug=True)
                tosend += end_batch
                self.send_over_bluetooth(tosend)
                self.devices_text_insert("[BT][TX] Sent end batch command (0x0D)", debug=True)

                self.devices_text_insert(f"[BT] Sent {len(self.schedule_queue)} schedule(s) to device.")
                self.schedule_queue.clear()

            else:
                self.devices_text_insert("Error: No valid connection type selected.")

        except Exception as e:
            self.devices_text_insert(f"Error sending schedules: {e}")
        
    def clear_schedule_queue(self):
        """Clear all queued schedules."""
        self.schedule_queue.clear()
        self.devices_text_insert("Schedule queue cleared.")

    def clear_device_schedule(self):
        """
        Clear the schedule stored on the connected device (UART or Bluetooth).

        This sends CLEARSCHEDULE (0x07), a dedicated command distinct from
        the SCHEDULECONTROL (0x05) used by Send Schedules. Sending an empty
        schedule via SCHEDULECONTROL isn't the same operation - the device's
        schedulemonth state always appends its in-progress entry when it
        sees the end-of-transmission byte, so a "start schedule then
        immediately end it" message would leave one bogus zeroed entry
        behind instead of a genuinely empty schedule. CLEARSCHEDULE clears
        and stops there, mirroring what the OLED's own menu does.

        This deletes every schedule entry on the device - local export/
        import files and the not-yet-sent queue in this GUI are unaffected.
        """
        if not self.ensure_device_connected():
            self.devices_text_insert("Error: No device connected.")
            return

        if not messagebox.askyesno(
            "Clear Device Schedule",
            "This will permanently erase the schedule stored on the connected "
            "speaker. This cannot be undone from the GUI. Continue?",
            icon="warning",
        ):
            self.devices_text_insert("Clear device schedule canceled.")
            return

        command = bytes([0x07])  # CLEARSCHEDULE

        if self.connection_type.get() == "UART" and self.serial_conn:
            try:
                self.devices_text_insert("[UART][TX] Sending clear schedule command (0x07)", debug=True)
                self.serial_conn.write(command)
                reply = self.serial_conn.read(1)
                if not reply:
                    self.devices_text_insert("[UART][ERROR] No response to clear schedule command.")
                elif reply[0] == 0:
                    self.devices_text_insert("[UART][ERROR] Device reported clear schedule failed.")
                else:
                    self.devices_text_insert("[UART] Device schedule cleared.")
            except Exception as e:
                self.devices_text_insert(f"[UART][ERROR] during clear schedule: {e}", debug=True)

        elif self.connection_type.get() == "Bluetooth" and self.device_connected:
            self.devices_text_insert("[BT][TX] Sending clear schedule command (0x07)", debug=True)
            # bluetooth_send() already confirms the command was actually
            # received by the device (raises/reports an error rather than
            # silently timing out - see its docstring), so there's no need
            # for a bespoke ack-byte read here on top of that; the device's
            # 1-byte success/fail reply still lands in its debug response
            # log if something needs a closer look.
            self.send_over_bluetooth(command)

        else:
            self.devices_text_insert("Error: No valid connection type selected.")

    def clear_device_log(self):
        """
        Clear the log stored on the connected device (UART or Bluetooth).

        Sends CLEARLOGS (0x08). Doesn't affect the schedule or anything
        currently playing - only the historical bucket-summary/boot-marker
        entries that Download Log reads. Whatever 2-hour bucket the device
        is currently mid-way through keeps accumulating normally and will
        be the first fresh entry once it closes.
        """
        if not self.ensure_device_connected():
            self.devices_text_insert("Error: No device connected.")
            return

        if not messagebox.askyesno(
            "Clear Device Log",
            "This will permanently erase the log stored on the connected "
            "speaker. This cannot be undone from the GUI. Continue?",
            icon="warning",
        ):
            self.devices_text_insert("Clear device log canceled.")
            return

        command = bytes([0x08])  # CLEARLOGS

        if self.connection_type.get() == "UART" and self.serial_conn:
            try:
                self.devices_text_insert("[UART][TX] Sending clear log command (0x08)", debug=True)
                self.serial_conn.write(command)
                reply = self.serial_conn.read(1)
                if not reply:
                    self.devices_text_insert("[UART][ERROR] No response to clear log command.")
                elif reply[0] == 0:
                    self.devices_text_insert("[UART][ERROR] Device reported clear log failed.")
                else:
                    self.devices_text_insert("[UART] Device log cleared.")
            except Exception as e:
                self.devices_text_insert(f"[UART][ERROR] during clear log: {e}", debug=True)

        elif self.connection_type.get() == "Bluetooth" and self.device_connected:
            self.devices_text_insert("[BT][TX] Sending clear log command (0x08)", debug=True)
            self.send_over_bluetooth(command)

        else:
            self.devices_text_insert("Error: No valid connection type selected.")

    def export_schedules(self):
        """
        Export the current schedule queue to a human-readable text file.

        Format (one per line):
            Schedule 1: 07/21-28 | 09:00-21:00 | Folder #1, File #2
        """
        if not self.schedule_queue:
            self.devices_text_insert("No schedules to export.")
            return

        try:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            default_filename = f"schedules_{timestamp}.txt"

            file_path = filedialog.asksaveasfilename(
                title="Export Schedules As",
                defaultextension=".txt",
                filetypes=[("Text Files", "*.txt")],
                initialfile=default_filename
            )

            if file_path:
                with open(file_path, "w", encoding="utf-8") as f:
                    f.write("# Wildlife Audio Player Schedule Export\n")
                    f.write(f"# Exported on: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                    f.write("# Edit the 'Schedule ...' lines directly to change times or tracks.\n")
                    f.write("# Format:\n")
                    f.write("#   Schedule N: MM/SD-ED | SH:SM-EH:EM | Folder #F, File #T\n")
                    f.write("# Example:\n")
                    f.write("#   Schedule 1: 07/21-28 | 09:00-21:00 | Folder #1, File #2\n\n")

                    for i, sched in enumerate(self.schedule_queue, 1):
                        line = (
                            f"Schedule {i}: "
                            f"{sched['month']:02d}/{sched['start_day']:02d}-{sched['end_day']:02d} | "
                            f"{sched['start_hour']:02d}:{sched['start_min']:02d}-"
                            f"{sched['stop_hour']:02d}:{sched['stop_min']:02d} | "
                            f"Folder #{sched['folder']}, File #{sched['file']}"
                        )
                        f.write(line + "\n")

                self.devices_text_insert(f"Exported {len(self.schedule_queue)} schedule(s) to {file_path}")
            else:
                self.devices_text_insert("Export canceled by user.")

        except Exception as e:
            self.devices_text_insert(f"Error exporting schedules: {str(e)}")

    def _validate_and_add_imported_schedule(
        self,
        month, start_day, start_hour, start_min,
        end_day, end_hour, end_min,
        folder, file, line_num=None
        ):
        """Validate imported schedule and add to queue if OK. Returns True if added."""
        valid_minutes = [0, 15, 30, 45]
        if start_min not in valid_minutes or end_min not in valid_minutes:
            if line_num is not None:
                self.devices_text_insert(
                    f"Warning: Line {line_num} has invalid minutes (use 00, 15, 30, 45), skipping."
                )
            return False

        if (end_hour, end_min) < (start_hour, start_min):
            if line_num is not None:
                self.devices_text_insert(
                    f"Warning: Line {line_num} has stop time before start time, skipping."
                )
            return False

        if start_day > end_day:
            if line_num is not None:
                self.devices_text_insert(
                    f"Warning: Line {line_num} has start day after end day, skipping."
                )
            return False

        if not (1 <= start_day <= 31) or not (1 <= end_day <= 31):
            if line_num is not None:
                self.devices_text_insert(
                    f"Warning: Line {line_num} has invalid day values, skipping."
                )
            return False

        new_entry = {
            "month": month,
            "start_day": start_day,
            "end_day": end_day,
            "start_hour": start_hour,
            "start_min": start_min,
            "stop_hour": end_hour,
            "stop_min": end_min,
            "folder": folder,
            "file": file
        }

        # Overlap detection (reuse your logic)
        for entry in self.schedule_queue:
            if entry["month"] == month:
                if not (end_day < entry["start_day"] or start_day > entry["end_day"]):
                    existing_start = (entry["start_hour"], entry["start_min"])
                    existing_stop  = (entry["stop_hour"], entry["stop_min"])
                    new_start      = (start_hour, start_min)
                    new_stop       = (end_hour, end_min)

                    if not (new_stop <= existing_start or new_start >= existing_stop):
                        if line_num is not None:
                            self.devices_text_insert(
                                f"Warning: Line {line_num} overlaps with existing schedule, skipping."
                            )
                        return False

        self.schedule_queue.append(new_entry)
        self.devices_text_insert(
            f"[Imported] {month:02d}/{start_day:02d}-{end_day:02d} | "
            f"{start_hour:02d}:{start_min:02d} - {end_hour:02d}:{end_min:02d} | "
            f"Folder #{folder}, File #{file}"
        )
        return True

    def import_schedules(self):
        """
        Import schedules from a text file.

        Preferred format (human-readable):
            Schedule 1: 07/21-28 | 09:00-21:00 | Folder #1, File #2

        Also supports legacy CSV lines:
            month,start_day,start_hour,start_min,end_day,end_hour,end_min,folder,file
        """
        try:
            file_path = filedialog.askopenfilename(
                title="Import Schedules From",
                filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")]
            )

            if not file_path:
                self.devices_text_insert("Import canceled by user.")
                return

            # Optional: clear existing queue so imported file becomes the new state
            self.schedule_queue.clear()
            self.devices_text_insert("Existing schedule queue cleared before import.")

            imported_count = 0

            # Regex for "Schedule N: MM/SD-ED | SH:SM-EH:EM | Folder #F, File #T"
            schedule_re = re.compile(
                r"^Schedule\s+\d+\s*:\s*"
                r"(\d{1,2})/(\d{1,2})-(\d{1,2})\s*\|\s*"
                r"(\d{1,2}):(\d{2})-(\d{1,2}):(\d{2})\s*\|\s*"
                r"Folder\s*#(\d+),\s*File\s*#(\d+)\s*$"
            )

            with open(file_path, "r", encoding="utf-8") as f:
                for line_num, line in enumerate(f, 1):
                    raw = line.strip()
                    if not raw:
                        continue

                    if raw.startswith("#"):
                        continue

                    # Try to parse as "Schedule ..." line (no more lstrip("# "))
                    m = schedule_re.match(raw)
                    if m:
                        try:
                            (month,
                            start_day,
                            end_day,
                            start_hour,
                            start_min,
                            end_hour,
                            end_min,
                            folder,
                            file) = map(int, m.groups())

                            if not self._validate_and_add_imported_schedule(
                                month, start_day, start_hour, start_min,
                                end_day, end_hour, end_min,
                                folder, file, line_num
                            ):
                                continue

                            imported_count += 1
                        except Exception as e:
                            self.devices_text_insert(
                                f"Warning: Line {line_num} could not be parsed as schedule ({e}), skipping."
                            )
                        continue  # don't fall through to CSV parsing

                    # Legacy CSV format (we already skipped comments above)
                    parts = raw.split(',')
                    if len(parts) == 9:
                        ...
                    else:
                        continue


            if imported_count > 0:
                self.devices_text_insert(
                    f"Successfully imported {imported_count} schedule(s) from {file_path}"
                )
            else:
                self.devices_text_insert("No valid schedules were imported.")

        except Exception as e:
            self.devices_text_insert(f"Error importing schedules: {str(e)}")


    def cleanup_resources(self):
        """
        Clean up all resources before closing the application.
        
        This method:
        1. Closes any open serial connections
        2. Disconnects any active Bluetooth connections
        3. Updates UI to reflect disconnected state
        4. Stops the asyncio event loop
        5. Handles any cleanup errors
        """
        try:
            # Close serial connection if open
            if self.serial_conn:
                self.serial_conn.close()
                self.serial_conn = None
            
            # Disconnect Bluetooth if connected
            if self.ble_client and self.ble_client.is_connected:
                try:
                    self.run_async(self.ble_client.disconnect())
                except Exception:
                    pass  # Ignore disconnect errors during cleanup
            
            # Stop the event loop
            if hasattr(self, 'loop') and self.loop.is_running():
                try:
                    self.loop.call_soon_threadsafe(self.loop.stop)
                    if hasattr(self, 'loop_thread'):
                        self.loop_thread.join(timeout=1.0)
                except Exception:
                    pass  # Ignore event loop cleanup errors
            
            # Update connection state
            self.device_connected = False
            
            # Try to update UI if window still exists
            try:
                if self.winfo_exists():
                    self.update_connection_status(False, error_message="Disconnected")
            except Exception:
                pass  # Ignore UI update errors during cleanup
                
        except Exception as e:
            print(f"Error during cleanup: {e}")

    def on_closing(self):
        """Handle window closing event."""
        try:
            # First update UI to show we're disconnecting
            if hasattr(self, 'connection_status_label') and self.connection_status_label.winfo_exists():
                self.update_connection_status(False, error_message="Disconnecting...")
        except Exception:
            pass  # Ignore UI update errors during closing
        
        # Then clean up resources
        self.cleanup_resources()
        
        # Finally destroy the window
        self.destroy()

    def __del__(self):
        """Fallback cleanup if on_closing wasn't called."""
        self.cleanup_resources()

    def update_connection_status(self, connected, connection_type=None, error_message=None, device_label=None):
        """
        Update the connection status display and control button states.
        
        Args:
            connected (bool): Whether the device is connected
            connection_type (str, optional): Type of connection (Bluetooth/UART)
            error_message (str, optional): Error message to display if disconnected
            device_label (str, optional): Name/address of the active device, shown
                so it's clear which of the (possibly several) visible speakers
                commands are actually being sent to
        """
        try:
            # Check if window is still valid
            if not self.winfo_exists():
                return
                
            if not hasattr(self, 'connection_status_label') or not self.connection_status_label.winfo_exists():
                return

            if connected:
                # Update UI for connected state
                status_color = "green"
                if connection_type and device_label:
                    status_text = f"● Connected via {connection_type} \u2014 {device_label}"
                elif connection_type:
                    status_text = f"● Connected via {connection_type}"
                else:
                    status_text = "● Connected"
                
                # Enable all control buttons when connected
                self.volume_set_button.config(state=tk.NORMAL)
                self.track_send_button.config(state=tk.NORMAL)
                self.duty_cycle_button.config(state=tk.NORMAL)
                self.add_entry_button.config(state=tk.NORMAL)
                self.send_all_button.config(state=tk.NORMAL)
                self.export_schedules_button.config(state=tk.NORMAL)
                self.import_schedules_button.config(state=tk.NORMAL)
                self.force_time_sync_button.config(state=tk.NORMAL)

                # Enable appropriate disconnect button and disable connect button
                if self.connection_type.get() == "UART":
                    self.uart_disconnect_button.config(state=tk.NORMAL)
                    self.uart_connect_button.config(state=tk.DISABLED)
                else:
                    self.bluetooth_disconnect_button.config(state=tk.NORMAL)
                    self.bluetooth_connect_button.config(state=tk.DISABLED)
                    self.scan_button.config(state=tk.DISABLED)
                
                # Update system time and date when connected - unless the
                # diagnostic "skip auto time sync" checkbox is on, in which
                # case skip it exactly once (so Check Time can see the
                # device's own unmodified RTC reading) and clear the
                # checkbox back off so it doesn't stay skipped by accident.
                if self.skip_auto_time_sync_var.get():
                    self.skip_auto_time_sync_var.set(False)
                    self.devices_text_insert("[TIME SYNC] Skipped auto time sync for this connect (diagnostic).", debug=True)
                else:
                    self.update_system_datetime()
            else:
                # Update UI for disconnected state
                status_color = "red"
                if error_message:
                    status_text = f"● Disconnected - {error_message}"
                else:
                    status_text = "● Disconnected"
                
                # Disable all control buttons when disconnected
                self.volume_set_button.config(state=tk.DISABLED)
                self.track_send_button.config(state=tk.DISABLED)
                self.duty_cycle_button.config(state=tk.DISABLED)
                self.add_entry_button.config(state=tk.DISABLED)
                self.send_all_button.config(state=tk.DISABLED)
                self.export_schedules_button.config(state=tk.DISABLED)
                self.import_schedules_button.config(state=tk.DISABLED)
                self.force_time_sync_button.config(state=tk.DISABLED)
                self.uart_disconnect_button.config(state=tk.DISABLED)
                self.bluetooth_disconnect_button.config(state=tk.DISABLED)
                
                # Enable appropriate connect button
                if self.connection_type.get() == "UART":
                    self.uart_connect_button.config(state=tk.NORMAL)
                else:
                    self.bluetooth_connect_button.config(state=tk.NORMAL)
                    self.scan_button.config(state=tk.NORMAL)
                
            # Update the status label with new text and color
            self.connection_status_label.config(text=status_text, foreground=status_color)
            self.device_connected = connected
        except Exception as e:
            print(f"Error updating connection status: {e}")
            # Don't re-raise the exception to prevent cascading errors

    def force_time_sync(self):
        """
        Manually re-push the Mac's current time to the connected device,
        on demand (not just the automatic sync-on-connect). Useful in the
        field to confirm the sync actually took, or to correct a device
        clock without having to disconnect/reconnect.
        """
        self.devices_text_insert("[TIME SYNC] Manual time sync requested...")
        self.update_system_datetime()

    def update_system_datetime(self):
        """
        Update the device's date and time with the current system time.
        This is called automatically when a connection is established, and
        can also be triggered on demand via force_time_sync/the "Force Time
        Sync" button.
        
        The time update packet format is:
        [0x0F, minute, hour, day, month]
        where:
        - minute: 0-59
        - hour: 0-23
        - day: 1-31
        - month: 1-12

        Note: for Bluetooth, actual success/failure of the write is only
        known once the background send thread finishes (see
        bluetooth_send/_run_bluetooth_send) - it now raises instead of
        silently reporting success when the device never confirms receipt,
        so a real failure shows up as a visible [BT][ERROR] line. For UART
        there is no equivalent confirmation in this protocol; the write is
        best-effort there.
        """
        if not self.ensure_device_connected():
            return

        try:
            # Get current system time
            now = datetime.now()
            
            # Format time components
            minute = now.minute
            hour = now.hour
            day = now.day     # 1-31
            month = now.month  # 1-12
            
            # Validate time components
            if not (0 <= minute <= 59):
                raise ValueError(f"Invalid minute: {minute}")
            if not (0 <= hour <= 23):
                raise ValueError(f"Invalid hour: {hour}")
            if not (1 <= day <= 31):
                raise ValueError(f"Invalid day: {day}")
            if not (1 <= month <= 12):
                raise ValueError(f"Invalid month: {month}")
            
            # Create command bytes for time update
            # Command 0x0F is used for time update with format: [0x0F, minute, hour, day, month]
            time_bytes = bytes([0x0F, minute, hour, day, month])
            
            if self.connection_type.get() == "UART" and self.serial_conn:
                self.devices_text_insert(f"[UART][TX] Updating system time: {hour:02d}:{minute:02d} Day:{day:02d} Month:{month:02d}", debug=True)
                self.serial_conn.write(time_bytes)
                
            elif self.connection_type.get() == "Bluetooth" and self.device_connected:
                self.devices_text_insert(f"[BT][TX] Updating system time: {hour:02d}:{minute:02d} Day:{day:02d} Month:{month:02d}", debug=True)
                self.send_over_bluetooth(time_bytes)
                
            self.devices_text_insert(
                f"[TIME SYNC] Sent {hour:02d}:{minute:02d} Day:{day:02d} Month:{month:02d} to device "
                f"(watch for a [BT][ERROR] below if the device doesn't confirm receipt)"
            )
            
        except ValueError as ve:
            self.devices_text_insert(f"Error: Invalid time value - {str(ve)}")
        except Exception as e:
            self.devices_text_insert(f"Error updating system time: {str(e)}")

    def connect_to_uart(self):
        """
        Connect to a device via UART (Serial) connection.
        
        This method:
        1. Validates the selected port and baud rate
        2. Attempts to establish a serial connection
        3. Updates UI with connection status
        4. Handles various error conditions
        """
        if self.connection_type.get() == "UART":
            try:
                # Validate port selection
                selection = self.serial_listbox.curselection()
                if not selection:
                    self.devices_text_insert("Error: No UART port selected.")
                    self.update_connection_status(False, error_message="No port selected")
                    return

                # Get selected port and baud rate
                index = selection[0]
                selected_port = self.serial_listbox.get(index)
                baudrate = self.baudrate_var.get()

                # Attempt connection
                self.update_connection_status(False, error_message="Connecting...")
                self.devices_text_insert(f"[UART] Attempting to connect to {selected_port} at {baudrate}...", debug=True)
                
                # Establish serial connection
                self.serial_conn = serial.Serial(selected_port, baudrate, timeout=1)
                self.device_connected = True
                self.update_connection_status(True, "UART")
                self.devices_text_insert(f"Connected to UART on {selected_port} at {baudrate} baud.")

                # Start monitoring for incoming data
                self.poll_uart_data()

            except serial.SerialException as e:
                # Handle serial-specific errors
                error_msg = str(e)
                self.devices_text_insert(f"[UART][ERROR] {error_msg}", debug=True)
                self.update_connection_status(False, error_message=error_msg)
            except Exception as e:
                # Handle other errors
                error_msg = str(e)
                self.devices_text_insert(f"[UART][ERROR] {error_msg}", debug=True)
                self.update_connection_status(False, error_message="Connection failed")
        else:
            self.devices_text_insert("Error: UART not selected as desired connection method.")

    def set_volume(self,volm):
        """
        Set the system volume (0-100%) and send command over UART or Bluetooth.
        """
        if not self.ensure_device_connected():
            return

        try:
            volume = volm

            if 0 <= volume <= 100:
                self.devices_text_insert(f"Volume set to: {volume}%")

                if self.connection_type.get() == "UART" and self.serial_conn:
                    self.devices_text_insert(f"[UART][TX] Sending volume command: 0x00 {volume}", debug=True)
                    self.serial_conn.write(bytes([0x00, volume]))

                elif self.connection_type.get() == "Bluetooth" and self.device_connected:
                    self.devices_text_insert(f"[BT][TX] Sending volume command: 0x00 {volume}", debug=True)
                    self.send_over_bluetooth(bytes([0x00, volume]))

                else:
                    self.devices_text_insert("Error: No valid connection.")

            else:
                self.devices_text_insert("Error: Volume must be between 0 and 100.")

        except ValueError:
            self.devices_text_insert("Error: Please enter a valid number for volume.")
        except Exception as e:
            self.devices_text_insert(f"Error setting volume: {str(e)}")

    def set_duty_cycle(self, val):
        """
        Set the system duty cycle (0-100%) and send command over UART or Bluetooth.
        """
        if not self.ensure_device_connected():
            return

        try:
            duty_cycle = val

            if 0 <= duty_cycle <= 100:
                self.devices_text_insert(f"Duty cycle set to: {duty_cycle}%")

                if self.connection_type.get() == "UART" and self.serial_conn:
                    self.devices_text_insert(f"[UART][TX] Sending duty cycle command: 0x04 {duty_cycle}", debug=True)
                    self.serial_conn.write(bytes([0x04, duty_cycle]))

                elif self.connection_type.get() == "Bluetooth" and self.device_connected:
                    self.devices_text_insert(f"[BT][TX] Sending duty cycle command: 0x04 {duty_cycle}", debug=True)
                    self.send_over_bluetooth(bytes([0x04, duty_cycle]))

                else:
                    self.devices_text_insert("Error: No valid connection.")

            else:
                self.devices_text_insert("Error: Duty cycle must be between 0 and 100.")

        except ValueError:
            self.devices_text_insert("Error: Please enter a valid number for duty cycle.")
        except Exception as e:
            self.devices_text_insert(f"Error setting duty cycle: {str(e)}")

    def send_folder_file(self, folder, file):
        """
        Send a specific folder and file selection command to the device.
        """
        if not self.ensure_device_connected():
            return

        try:
            folder = int(folder)
            file = int(file)

            if 0 <= folder <= 255 and 0 <= file <= 255:
                self.devices_text_insert(f"Sending Folder #{folder}, File #{file}")

                if self.connection_type.get() == "UART" and self.serial_conn:
                    self.devices_text_insert(f"[UART][TX] Sending folder/file command: 0x01 {folder} {file}", debug=True)
                    self.serial_conn.write(bytes([0x01, folder, file]))

                elif self.connection_type.get() == "Bluetooth" and self.device_connected:
                    self.devices_text_insert(f"[BT][TX] Sending folder/file command: 0x01 {folder} {file}", debug=True)
                    self.send_over_bluetooth(bytes([0x01, folder, file]))

                else:
                    self.devices_text_insert("Error: No valid connection.")

            else:
                self.devices_text_insert("Error: Folder and File must be between 0 and 255.")

        except ValueError:
            self.devices_text_insert("Error: Invalid folder or file number. Please enter valid integers.")
        except Exception as e:
            self.devices_text_insert(f"Error sending folder/file: {str(e)}")

    def disconnect_device(self):
        """Disconnect from the current device and clean up resources."""
        try:
            if self.connection_type.get() == "UART" and self.serial_conn:
                self.serial_conn.close()
                self.serial_conn = None
            elif self.connection_type.get() == "Bluetooth" and self.ble_client:
                # Run cleanup in async context
                self.run_async(self._cleanup_connection())
            
            self.device_connected = False
            self.ble_device = None
            self.active_device_address = None
            self._refresh_device_listbox_highlight()
            self.update_connection_status(False, error_message="Disconnected")
            
            # Update button states
            if self.connection_type.get() == "UART":
                self.uart_disconnect_button.config(state=tk.DISABLED)
            else:
                self.bluetooth_disconnect_button.config(state=tk.DISABLED)
                
            self.devices_text_insert(f"Disconnected from {self.connection_type.get()} device.")
            
        except Exception as e:
            self.devices_text_insert(f"Error during disconnect: {str(e)}")

# Main application entry point
if __name__ == "__main__":
    app = AmbianceGUI()
    app.mainloop()
