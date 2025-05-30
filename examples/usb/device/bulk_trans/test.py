import usb.core
import usb.util
import time

VID = 0xCAFE
PID = 0x4001

# Find device
dev = usb.core.find(idVendor=VID, idProduct=PID)
if dev is None:
    raise ValueError('Device not found')

print('Device found!')

# Check if device is already opened
try:
    # Try to get current configuration
    current_cfg = dev.get_active_configuration()
    print('Device is already configured')
except usb.core.USBError:
    # If getting configuration fails, device is not opened, need to set configuration
    print('Device is not configured, setting configuration...')
    dev.set_configuration()
    current_cfg = dev.get_active_configuration()

# Get interface
intf = current_cfg[(0, 0)]

# Find endpoints
ep_out = usb.util.find_descriptor(
    intf,
    custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT
)
ep_in = usb.util.find_descriptor(
    intf,
    custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN
)

assert ep_out is not None, 'OUT endpoint not found'
assert ep_in is not None, 'IN endpoint not found'

# Send and receive data
try:
    # Send fixed data
    message = b'Hello ESP32!'
    print('Sending:', message)
    ep_out.write(message)

    # Receive data
    try:
        data = ep_in.read(64, timeout=1000)  # Read up to 64 bytes, timeout 1 second
        print('Received:', data.tobytes().decode('utf-8'))
    except usb.core.USBTimeoutError:
        print('No response received (timeout)')
    except Exception as e:
        print('Error receiving data:', str(e))

    # Close device
    print('Closing device...')
    usb.util.release_interface(dev, intf)
    usb.util.dispose_resources(dev)
    print('Device closed successfully')

except Exception as e:
    print('Error:', str(e))
    # Ensure device is closed on error
    try:
        usb.util.release_interface(dev, intf)
        usb.util.dispose_resources(dev)
        print('Device closed successfully')
    except:
        pass
