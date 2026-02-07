import socket
import struct
from pynput.mouse import Button, Controller

mouse=Controller()
# UDP settings
UDP_IP = "0.0.0.0"  # Listen on all network interfaces
UDP_PORT = 1337      # Must match ESP8266 port

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening for UDP packets on port {UDP_PORT}...")
print("Make sure your firewall allows this port!")
print("-" * 50)

while True:
    try:
        # Receive data
        data, addr = sock.recvfrom(1024)  # Buffer size 1024 bytes
        
        # Decode the message
        message = data.decode('utf-8').strip()
        
        print(f"Received {message}")
        
        # Parse the data
        if ',' in message:
            parts = message.split(',')
            if len(parts) == 2:
                raw_avg = int(parts[0])
                flex = int(parts[1])
                if flex <100:
                    #Simulate click
                    mouse.press(Button.left)
                    mouse.release(Button.left)
                    
                
    except KeyboardInterrupt:
        print("\nStopping receiver...")
        break
    except Exception as e:
        print(f"Error: {e}")
        continue

sock.close()
print("Receiver stopped.")