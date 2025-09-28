"""
Ambiance GUI Application

This application provides a graphical interface for controlling audio playback devices
via Bluetooth or UART connections. It supports:
- Device connection management
- Audio control (volume, track selection)
- Schedule management
- Logging and monitoring
"""

import sys
import serial
import asyncio
import threading
import serial.tools.list_ports
from bleak import BleakScanner, BleakClient
import tkinter as tk
from tkinter import ttk, filedialog
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

        # get screen width and height
        ws = self.winfo_screenwidth() # width of the screen
        hs = self.winfo_screenheight() # height of the screen
        window_w= int(ws*.45)
        window_h= hs*.907
        x = (ws/2) - (window_w/2) # X-axis offset of window
        y = (hs/2) - (window_h/2) #Y-axis offset of window
        self.geometry('%dx%d+%d+%d' % (window_w, window_h, x, 0))# Adjusted window size


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

        # Scan button
        self.scan_button = ttk.Button(
            bluetooth_frame,
            text="Scan for Devices",
            command=self.start_scan_devices
        )
        self.scan_button.pack(pady=7, padx=10)

        # Device list label
        self.devices_label = ttk.Label(bluetooth_frame, text="Discovered Devices:")
        self.devices_label.pack(padx=10)

        # Device listbox
        self.devices_listbox = tk.Listbox(bluetooth_frame, height=5)
        self.devices_listbox.pack(pady=5, padx=10, fill="x")

        # Create frame for connect/disconnect buttons
        button_frame = ttk.Frame(bluetooth_frame)
        button_frame.pack(pady=5, padx=10, fill="x")

        # Connect button
        self.bluetooth_connect_button = ttk.Button(
            button_frame,
            text="Connect",
            command=self.connect_to_bluetooth
        )
        self.bluetooth_connect_button.pack(side=tk.LEFT, padx=(0, 5), expand=True)

        # Disconnect button
        self.bluetooth_disconnect_button = ttk.Button(
            button_frame,
            text="Disconnect",
            command=self.disconnect_device,
            state=tk.DISABLED
        )
        self.bluetooth_disconnect_button.pack(side=tk.LEFT, padx=(5, 0), expand=True)

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
        
        self.volume_input = ttk.Entry(volume_input_frame, width=8)
        self.volume_input.pack()
        
        # Volume set button below
        self.volume_set_button = ttk.Button(
            volume_section,
            text="Set",
            command=self.set_volume
        )
        self.volume_set_button.pack(pady=(0, 10), padx=10)

        # Duty cycle control (center)
        duty_section = ttk.LabelFrame(controls_row, text="Duty Cycle")
        duty_section.pack(side=tk.LEFT, fill="both", expand=True, padx=(0, 10))

        # Duty cycle input field
        duty_input_frame = ttk.Frame(duty_section)
        duty_input_frame.pack(pady=7, padx=10)
        
        self.duty_cycle_input = ttk.Entry(duty_input_frame, width=8)
        self.duty_cycle_input.pack()
        
        # Duty cycle set button below
        self.duty_cycle_button = ttk.Button(
            duty_section,
            text="Set",
            command=self.set_duty_cycle
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
        scheduler_frame.pack(fill="x", pady=7, padx=10)

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
        self.month_entry = ttk.Entry(date_grid, width=8)
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
        
        self.start_hour_entry = ttk.Entry(start_time_frame, width=4)
        self.start_hour_entry.pack(side=tk.LEFT)
        ttk.Label(start_time_frame, text=":").pack(side=tk.LEFT, padx=2)
        self.start_min_entry = ttk.Entry(start_time_frame, width=4)
        self.start_min_entry.pack(side=tk.LEFT)

        # Stop time controls
        ttk.Label(time_grid, text="Stop Time:").grid(row=1, column=0, padx=(0, 5), pady=5, sticky="e")
        stop_time_frame = ttk.Frame(time_grid)
        stop_time_frame.grid(row=1, column=1, padx=(0, 15), pady=5, sticky="w")
        
        self.stop_hour_entry = ttk.Entry(stop_time_frame, width=4)
        self.stop_hour_entry.pack(side=tk.LEFT)
        ttk.Label(stop_time_frame, text=":").pack(side=tk.LEFT, padx=2)
        self.stop_min_entry = ttk.Entry(stop_time_frame, width=4)
        self.stop_min_entry.pack(side=tk.LEFT)

        # Time format hint
        time_hint_label = ttk.Label(
            time_frame,
            text="(Hours: 0-23, Minutes: 00, 15, 30, 45)",
            foreground="gray",
            font=("Arial", 8)
        )
        time_hint_label.pack(pady=(0, 5))

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

    def _setup_log_section(self):
        """Set up the log and output section."""
        # Create frame for log controls
        bottom_button_frame = ttk.Frame(self.frame)
        bottom_button_frame.pack(fill="both", padx=10, pady=5)

        # Download log button
        self.download_log_button = ttk.Button(
            bottom_button_frame,
            text="Download Log",
            command=self.download_log
        )
        self.download_log_button.pack(side=tk.LEFT, padx=5)

        # Clear output button
        self.clear_button = ttk.Button(
            bottom_button_frame,
            text="Clear Output",
            command=self.clear_textbox
        )
        self.clear_button.pack(side=tk.RIGHT, padx=5)

        # Create text display area
        self._setup_text_display()

    def _setup_text_display(self):
        """Set up the text display area for logs and messages."""
        # Create frame for text display
        text_frame = ttk.Frame(self.frame)
        text_frame.pack(fill="both", expand=True, padx=5, pady=5)

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
            if device.name:
                self.devices_listbox.insert(tk.END, device.name)
        self.devices_text_insert(f"[BT] Found {len(devices)} devices during scan", debug=True)

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
            
            # Log detailed information about each device
            for device in devices:
                if device.name:
                    self.devices_text_insert(
                        f"[BT] Found device: {device.name}",
                        debug=True
                    )
            
            return devices
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
            selection = self.devices_listbox.curselection()
            if not selection:
                self.devices_text_insert("[BT][ERROR] No Bluetooth device selected.")
                return

            index = selection[0]
            selected_device_name = self.devices_listbox.get(index)
            self.devices_text_insert(f"[BT] Attempting to connect to {selected_device_name}...", debug=True)

            # Disable connect button
            self.bluetooth_connect_button.config(state=tk.DISABLED)

            # Start connection in a separate thread
            threading.Thread(target=self._run_bluetooth_connection, args=(selected_device_name,), daemon=True).start()

    def _run_bluetooth_connection(self, device_name):
        """
        Run the Bluetooth connection process in a separate thread.
        
        Args:
            device_name (str): Name of the Bluetooth device to connect to
        """
        try:
            self.run_async(self.async_connect_to_bluetooth(device_name))
        except Exception as e:
            error_msg = str(e)  # Capture the error message
            self.after(0, lambda: self.devices_text_insert(f"[BT][ERROR] Connection failed: {error_msg}"))
            self.after(0, lambda: self.update_connection_status(False, error_message="Connection failed"))
        finally:
            # Re-enable connect button
            self.after(0, lambda: self.bluetooth_connect_button.config(state=tk.NORMAL))

    async def async_connect_to_bluetooth(self, device_name):
        """
        Asynchronous Bluetooth connection logic.
        
        Args:
            device_name (str): Name of the Bluetooth device to connect to
        """
        try:
            # Check if we have discovered devices
            if not hasattr(self, 'discovered_devices') or not self.discovered_devices:
                # If no devices are stored, do a quick scan
                self.after(0, lambda: self.devices_text_insert("[BT] Scanning for devices...", debug=True))
                self.discovered_devices = await BleakScanner.discover(timeout=self.scan_timeout)
                self.after(0, lambda: self.devices_text_insert(f"[BT] Found {len(self.discovered_devices)} devices during scan", debug=True))
            
            # Get the device from the list of discovered devices
            device_found = False
            for device in self.discovered_devices:
                if device.name == device_name:
                    device_found = True
                    self.ble_device = device
                    self.after(0, lambda: self.devices_text_insert(f"[BT] Found device: {device_name}", debug=True))
                    
                    # Create client with timeout
                    self.ble_client = BleakClient(device, timeout=30.0)

                    # Attempt connection
                    self.after(0, lambda: self.update_connection_status(False, error_message="Connecting..."))
                    self.after(0, lambda: self.devices_text_insert(f"[BT] Connecting to {device_name}...", debug=True))
                    
                    try:
                        await asyncio.wait_for(self.ble_client.connect(), timeout=30.0)
                        self.after(0, lambda: self.devices_text_insert("[BT] Connected, verifying services...", debug=True))
                    except asyncio.TimeoutError:
                        raise Exception("Connection attempt timed out after 30 seconds")
                    except Exception as conn_error:
                        raise Exception(f"Failed to establish connection: {str(conn_error)}")
                    
                    # Verify service and characteristics
                    services = self.ble_client.services
                    service_uuids = [s.uuid for s in services]
                    
                    if self.ble_service_uuid not in service_uuids:
                        raise Exception(f"Device does not have required USART service")
                    
                    service = next(s for s in services if s.uuid == self.ble_service_uuid)
                    characteristics = [c.uuid for c in service.characteristics]
                    required_chars = [self.ble_tx_uuid, self.ble_rx_uuid, self.ble_req_tx_uuid]
                    
                    if not all(char in characteristics for char in required_chars):
                        raise Exception("Device does not have all required USART characteristics - incompatible device")
                    
                    # Connection successful - no notifications needed
                    self.device_connected = True
                    self.connection_retry_count = 0  # Reset retry count on successful connection
                    self.after(0, lambda: self.update_connection_status(True, "Bluetooth"))
                    self.after(0, lambda: self.devices_text_insert(f"[BT] Connected successfully to {device_name}.", debug=True))
                    return

            # Handle device not found
            if not device_found:
                self.after(0, lambda: self.update_connection_status(False, error_message="Device not found"))
                self.after(0, lambda: self.devices_text_insert(f"[BT][ERROR] Device '{device_name}' not found in discovered devices."))
                
        except asyncio.TimeoutError:
            # Handle scan timeout
            self.after(0, lambda: self.update_connection_status(False, error_message="Connection timeout"))
            self.after(0, lambda: self.devices_text_insert("[BT][ERROR] Connection timed out. Please try again."))
        except Exception as e:
            # Handle other errors
            error_msg = str(e)
            self.after(0, lambda: self.update_connection_status(False, error_message=error_msg))
            self.after(0, lambda: self.devices_text_insert(f"[BT][ERROR] during connection: {error_msg}", debug=True))
            
            # Clean up on failure
            await self._cleanup_connection()
            
            # Check if this is a characteristic compatibility error (don't retry)
            if ("incompatible device" in error_msg.lower() or
                "required USART service" in error_msg.lower() or
                "required USART characteristics" in error_msg.lower() or
                "Device does not have required USART service" in error_msg):
                self.after(0, lambda: self.devices_text_insert("[BT][ERROR] Device is incompatible - no retries will be attempted.", debug=True))
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
                self.after(0, lambda: self.devices_text_insert(f"[BT] Warning: Error during cleanup: {str(e)}", debug=True))
            finally:
                self.ble_client = None
                self.device_connected = False

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
                self.after(0, lambda: self.devices_text_insert(f"[BT][RX] Data written to RX: {list(byte)}", debug=True))
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
                    # Log read errors for debugging
                    self.after(0, lambda: self.devices_text_insert(f"[BT][TX] No data available on read attempt {attempt}", debug=True))
                
                # Then check TX_REQ status
                try:
                    tx_req = await self.ble_client.read_gatt_char(self.ble_req_tx_uuid)
                    req_value = tx_req[0]
                    
                    if last_tx_req != req_value:
                        self.after(0, lambda: self.devices_text_insert(f"[BT][TX_REQ] State changed from {last_tx_req} to {req_value}", debug=True))
                        last_tx_req = req_value
                    
                    if req_value == 2:
                        # Transmission complete
                        self.after(0, lambda: self.devices_text_insert("[BT][TX_REQ] Transmission complete signal received", debug=True))
                        break
                except Exception as e:
                    self.after(0, lambda: self.devices_text_insert(f"[BT][TX_REQ] Error reading state: {str(e)}", debug=True))
                
                # Wait a short time before next poll
                await asyncio.sleep(0.01)  # 10ms delay between polls
                attempt += 1
                
                if attempt % 5 == 0:
                    self.after(0, lambda: self.devices_text_insert(f"[BT] Polling attempt {attempt}/{max_attempts}", debug=True))
            
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
            
            self.after(0, lambda: self.devices_text_insert("[BT][TX] Communication complete", debug=True))
        except Exception as e:
            raise Exception(f"Failed to send data: {str(e)}")

    def _attempt_reconnect(self):
        """Attempt to reconnect to the Bluetooth device."""
        if self.ble_device and not self.device_connected:
            self.devices_text_insert("[BT] Attempting to reconnect...", debug=True)
            threading.Thread(target=self._run_bluetooth_connection, args=(self.ble_device.name,), daemon=True).start()

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

    def download_log(self):
        """Request and download system log from the device (UART or Bluetooth)."""
        if not self.ensure_device_connected():
            self.devices_text_insert("Error: No device connected.")
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

                size = (high[0] << 8) | low[0]
                self.devices_text_insert(f"[UART][RX] Log size received: {size} bytes", debug=True)

                received_data = b""
                while len(received_data) < size:
                    chunk = self.serial_conn.read(size - len(received_data))
                    if not chunk:
                        break
                    received_data += chunk
                    self.devices_text_insert(f"[UART][RX] Received {len(received_data)} / {size} bytes...", debug=True)

                log_text = received_data.decode(errors="replace")
                self.preview_and_save_log(log_text)

            except Exception as e:
                self.devices_text_insert(f"[UART][ERROR] during log download: {e}", debug=True)

        elif self.connection_type.get() == "Bluetooth" and self.device_connected:
            try:
                # Bluetooth Download Logic
                self.devices_text_insert("[BT][TX] Sending log request command: 0x02", debug=True)
                self.send_over_bluetooth(bytes([0x02]))

                # Wait for device to send size (2 bytes) over BLE
                async def ble_download():
                    if self.ble_client and self.ble_client.is_connected:
                        # Read size bytes
                        high = await self.ble_client.read_gatt_char(self.ble_tx_uuid)
                        await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))  # Acknowledge high byte
                        low = await self.ble_client.read_gatt_char(self.ble_tx_uuid)
                        await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))  # Acknowledge low byte

                        if not high or not low:
                            self.devices_text_insert("[BT][ERROR] Failed to receive log size.")
                            return

                        size = (high[0] << 8) | low[0]
                        self.devices_text_insert(f"[BT][RX] Log size received: {size} bytes", debug=True)

                        received_data = b""
                        while len(received_data) < size:
                            chunk = await self.ble_client.read_gatt_char(self.ble_tx_uuid)
                            if not chunk:
                                break
                            received_data += chunk
                            # Acknowledge each chunk received
                            await self.ble_client.write_gatt_char(self.ble_req_tx_uuid, bytes([1]))
                            self.devices_text_insert(f"[BT][RX] Received {len(received_data)} / {size} bytes...", debug=True)

                        log_text = received_data.decode(errors="replace")
                        self.preview_and_save_log(log_text)
                    else:
                        self.devices_text_insert("[BT][ERROR] No BLE connection active.")

                threading.Thread(target=lambda: asyncio.run(ble_download())).start()

            except Exception as e:
                self.devices_text_insert(f"[BT][ERROR] during log download: {e}", debug=True)

        else:
            self.devices_text_insert("Error: Log download only supported over UART or Bluetooth.")

    def preview_and_save_log(self, log_text):
        """Helper to preview and save downloaded log."""
        preview = log_text[:300] + ("..." if len(log_text) > 300 else "")
        self.devices_text_insert("Log Preview:\n" + preview)

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        default_filename = f"log_{timestamp}.txt"

        file_path = filedialog.asksaveasfilename(
            title="Save Log As",
            defaultextension=".txt",
            filetypes=[("Text Files", "*.txt")],
            initialfile=default_filename
        )

        if file_path:
            with open(file_path, "w", encoding="utf-8") as f:
                f.write(log_text)
            self.devices_text_insert(f"Log saved to {file_path}")
        else:
            self.devices_text_insert("Save canceled by user.")

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

    def export_schedules(self):
        """
        Export the current schedule queue to a text file.
        
        This function allows users to save their current schedule configuration
        to a text file for backup, sharing, or copying to other devices.
        
        File Format:
        - Each schedule is stored as a single line of comma-separated values
        - Format: month,start_day,start_hour,start_min,end_day,end_hour,end_min,folder,file
        - Example: 7,21,9,0,28,21,0,1,2 (July 21-28, 9:00 AM to 9:00 PM, Folder 1, File 2)
        
        File Structure:
        - Header comments explaining the format
        - Timestamp of when the export was created
        - One line per schedule with data values
        - Human-readable description for each schedule
        
        Usage:
        1. Ensure you have schedules in the queue
        2. Click "Export Schedules" button
        3. Choose save location and filename
        4. File will be created with all current schedules
        
        The exported file can be imported on other devices using the Import function.
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
                    f.write("# Format: month,start_day,start_hour,start_min,end_day,end_hour,end_min,folder,file\n")
                    f.write("# Example: 7,21,9,0,28,21,0,1,2\n\n")
                    
                    for i, sched in enumerate(self.schedule_queue, 1):
                        # Format: month,start_day,start_hour,start_min,end_day,end_hour,end_min,folder,file
                        line = f"{sched['month']},{sched['start_day']},{sched['start_hour']},{sched['start_min']},{sched['end_day']},{sched['stop_hour']},{sched['stop_min']},{sched['folder']},{sched['file']}"
                        f.write(line + "\n")
                        
                        # Also write a human-readable description
                        f.write(f"# Schedule {i}: {sched['month']:02d}/{sched['start_day']:02d}-{sched['end_day']:02d} | {sched['start_hour']:02d}:{sched['start_min']:02d} - {sched['stop_hour']:02d}:{sched['stop_min']:02d} | Folder #{sched['folder']}, File #{sched['file']}\n\n")

                self.devices_text_insert(f"Exported {len(self.schedule_queue)} schedule(s) to {file_path}")
            else:
                self.devices_text_insert("Export canceled by user.")

        except Exception as e:
            self.devices_text_insert(f"Error exporting schedules: {str(e)}")

    def import_schedules(self):
        """
        Import schedules from a text file.
        
        This function allows users to load schedule configurations from a previously
        exported text file, enabling easy copying of schedules between devices.
        
        File Format Expected:
        - Each schedule should be on a single line with comma-separated values
        - Format: month,start_day,start_hour,start_min,end_day,end_hour,end_min,folder,file
        - Lines starting with '#' are treated as comments and ignored
        - Empty lines are ignored
        
        Validation Performed:
        - Ensures each line has exactly 9 comma-separated values
        - Validates that all values are valid integers
        - Checks that minutes are valid (0, 15, 30, 45)
        - Ensures stop time is after start time
        - Ensures start day is before or equal to end day
        - Validates day ranges (1-31)
        - Checks for overlaps with existing schedules in the queue
        
        Error Handling:
        - Invalid lines are skipped with warning messages
        - Overlapping schedules are skipped with warning messages
        - Continues processing even if some lines fail
        - Provides summary of successful imports
        
        Usage:
        1. Click "Import Schedules" button
        2. Select a previously exported schedule file
        3. Function will validate and import valid schedules
        4. Check the log for import results and any warnings
        
        The imported schedules are added to the current queue and can be sent
        to the device using the "Send Schedules" button.
        """
        try:
            file_path = filedialog.askopenfilename(
                title="Import Schedules From",
                filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")]
            )

            if not file_path:
                self.devices_text_insert("Import canceled by user.")
                return

            imported_count = 0
            with open(file_path, "r", encoding="utf-8") as f:
                for line_num, line in enumerate(f, 1):
                    line = line.strip()
                    
                    # Skip empty lines and comments
                    if not line or line.startswith('#'):
                        continue
                    
                    try:
                        # Parse the comma-separated values
                        parts = line.split(',')
                        if len(parts) != 9:
                            self.devices_text_insert(f"Warning: Line {line_num} has incorrect format, skipping.")
                            continue
                        
                        month = int(parts[0])
                        start_day = int(parts[1])
                        start_hour = int(parts[2])
                        start_min = int(parts[3])
                        end_day = int(parts[4])
                        end_hour = int(parts[5])
                        end_min = int(parts[6])
                        folder = int(parts[7])
                        file = int(parts[8])

                        # Validate the data
                        valid_minutes = [0, 15, 30, 45]
                        if start_min not in valid_minutes or end_min not in valid_minutes:
                            self.devices_text_insert(f"Warning: Line {line_num} has invalid minutes, skipping.")
                            continue

                        if (end_hour, end_min) <= (start_hour, start_min):
                            self.devices_text_insert(f"Warning: Line {line_num} has stop time before start time, skipping.")
                            continue

                        if start_day > end_day:
                            self.devices_text_insert(f"Warning: Line {line_num} has start day after end day, skipping.")
                            continue

                        if not (1 <= start_day <= 31) or not (1 <= end_day <= 31):
                            self.devices_text_insert(f"Warning: Line {line_num} has invalid day values, skipping.")
                            continue

                        # Create the schedule entry
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

                        # Check for overlaps with existing schedules
                        overlap_found = False
                        for entry in self.schedule_queue:
                            if entry["month"] == month:
                                # Check if day ranges overlap
                                if not (end_day < entry["start_day"] or start_day > entry["end_day"]):
                                    # Day ranges overlap, check if time ranges also overlap
                                    existing_start = (entry["start_hour"], entry["start_min"])
                                    existing_stop = (entry["stop_hour"], entry["stop_min"])
                                    new_start = (start_hour, start_min)
                                    new_stop = (end_hour, end_min)
                                    
                                    # Check if time ranges overlap
                                    if not (new_stop <= existing_start or new_start >= existing_stop):
                                        self.devices_text_insert(f"Warning: Line {line_num} overlaps with existing schedule, skipping.")
                                        overlap_found = True
                                        break

                        if not overlap_found:
                            self.schedule_queue.append(new_entry)
                            imported_count += 1
                            self.devices_text_insert(
                                f"[Imported] {month:02d}/{start_day:02d}-{end_day:02d} | {start_hour:02d}:{start_min:02d} - "
                                f"{end_hour:02d}:{end_min:02d} | Folder #{folder}, File #{file}"
                            )

                    except ValueError as e:
                        self.devices_text_insert(f"Warning: Line {line_num} has invalid numbers, skipping.")
                    except Exception as e:
                        self.devices_text_insert(f"Warning: Error processing line {line_num}: {str(e)}")

            if imported_count > 0:
                self.devices_text_insert(f"Successfully imported {imported_count} schedule(s) from {file_path}")
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

    def update_connection_status(self, connected, connection_type=None, error_message=None):
        """
        Update the connection status display and control button states.
        
        Args:
            connected (bool): Whether the device is connected
            connection_type (str, optional): Type of connection (Bluetooth/UART)
            error_message (str, optional): Error message to display if disconnected
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
                if connection_type:
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

                # Enable appropriate disconnect button and disable connect button
                if self.connection_type.get() == "UART":
                    self.uart_disconnect_button.config(state=tk.NORMAL)
                    self.uart_connect_button.config(state=tk.DISABLED)
                else:
                    self.bluetooth_disconnect_button.config(state=tk.NORMAL)
                    self.bluetooth_connect_button.config(state=tk.DISABLED)
                    self.scan_button.config(state=tk.DISABLED)
                
                # Update system time and date when connected
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

    def update_system_datetime(self):
        """
        Update the device's date and time with the current system time.
        This is called automatically when a connection is established.
        
        The time update packet format is:
        [0x0F, minute, hour, day, month]
        where:
        - minute: 0-59
        - hour: 0-23
        - day: 1-31
        - month: 1-12
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
                
            self.devices_text_insert(f"System time updated to {hour:02d}:{minute:02d} Day:{day:02d} Month:{month:02d}")
            
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

    def set_volume(self):
        """
        Set the system volume (0-100%) and send command over UART or Bluetooth.
        """
        if not self.ensure_device_connected():
            return

        try:
            volume = int(self.volume_input.get())

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

    def set_duty_cycle(self):
        """
        Set the system duty cycle (0-100%) and send command over UART or Bluetooth.
        """
        if not self.ensure_device_connected():
            return

        try:
            duty_cycle = int(self.duty_cycle_input.get())

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
