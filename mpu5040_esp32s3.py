import serial
import time
from pynput.mouse import Controller
from collections import deque

# ===== CONFIG =====
SERIAL_PORT = "/dev/ttyACM0"  # change to your port
BAUD = 115200

# Reduced base sensitivity
sensitivity_x = 0.20    # Reduced from 0.1
sensitivity_y = 0.21     # Reduced from 0.2

# NEW: Additional smoothing parameters
SMOOTHING_ALPHA = 0.8  # Exponential moving average (0-1, higher = smoother)
USE_MOVING_AVERAGE = True
MOVING_AVG_WINDOW_x = 5   # Average last N samples
MOVING_AVG_WINDOW_y = 2   # Average last N samples

# NEW: Axis prioritization settings
ENABLE_AXIS_PRIORITY = True
AXIS_THRESHOLD = 1.58    # How much stronger one axis needs to be (1.5 = 50% stronger)
SUPPRESSION_FACTOR = 0.2 # How much to reduce the weaker axis (0.3 = reduce to 30%)

mouse = Controller()
ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)
time.sleep(2)  # allow ESP8266 to reset

# Increased deadzone
deadzone = 7  # Increased from 5

# NEW: Smoothing buffers
smooth_x = 0.0
smooth_y = 0.0

# NEW: Moving average buffers
if USE_MOVING_AVERAGE:
    buffer_x = deque(maxlen=MOVING_AVG_WINDOW_x)
    buffer_y = deque(maxlen=MOVING_AVG_WINDOW_y)

print("Starting MPU6050 mouse control...")

while True:
    if ser.in_waiting:
        try:
            line = ser.readline().decode().strip()
            print(f"Received: {line}")  # Debug: print raw data
            
            if line:
                parts = line.split(',')
                if len(parts) == 3:
                    dx = float(parts[0])
                    dy = float(parts[1])
                    dz = float(parts[2])
                    
                    # Calculate movement
                    move_x = dy * sensitivity_y if abs(dy) > deadzone*1.4 else 0
                    
                    move_y = -dx * sensitivity_x if abs(dz) > deadzone*1 else 0
                    if move_y > 0: move_y*=1.1
                    
                    # NEW: Apply exponential moving average smoothing
                    smooth_x = SMOOTHING_ALPHA * smooth_x + (1 - SMOOTHING_ALPHA) * move_x
                    smooth_y = SMOOTHING_ALPHA * smooth_y + (1 - SMOOTHING_ALPHA) * move_y
                    
                    # NEW: Optional moving average
                    if USE_MOVING_AVERAGE:
                        buffer_x.append(smooth_x)
                        buffer_y.append(smooth_y)
                        
                        final_x = sum(buffer_x) / len(buffer_x) if len(buffer_x) > 0 else 0
                        final_y = sum(buffer_y) / len(buffer_y) if len(buffer_y) > 0 else 0
                    else:
                        final_x = smooth_x
                        final_y = smooth_y
                    
                    # NEW: Axis prioritization - focus on stronger movement
                    if ENABLE_AXIS_PRIORITY:
                        abs_x = abs(final_x)
                        abs_y = abs(final_y)
                        
                        # If both movements exist
                        if abs_x > 0.05 and abs_y > 0.05:
                            # Calculate ratio
                            if abs_x > abs_y:
                                ratio = abs_x / abs_y if abs_y > 0 else 999
                                # If X is significantly stronger, reduce Y
                                if ratio > AXIS_THRESHOLD:
                                    final_y *= SUPPRESSION_FACTOR
                                    print(f"Prioritizing X axis (ratio: {ratio:.2f})")
                            else:
                                ratio = abs_y / abs_x if abs_x > 0 else 999
                                # If Y is significantly stronger, reduce X
                                if ratio > AXIS_THRESHOLD:
                                    final_x *= SUPPRESSION_FACTOR
                                    print(f"Prioritizing Y axis (ratio: {ratio:.2f})")
                    
                    # NEW: Apply final deadzone on smoothed values
                    if abs(final_x) < 0.05:
                        final_x = 0
                    if abs(final_y) < 0.05:
                        final_y = 0
                    
                    # Move mouse with smoothed values
                    if final_x != 0 or final_y != 0:
                        mouse.move(int(final_x), int(final_y))
                        
        except Exception as e:
            print(f"Error: {e}")
            continue