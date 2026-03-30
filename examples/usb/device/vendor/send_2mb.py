#!/usr/bin/env python3
"""Send 2MB of data to ESP32 vendor USB device and verify transfer."""

import usb.core
import usb.util
import time

USB_VID = 0xcafe
USB_PID = 0x4001
TARGET_SIZE = 2 * 1024 * 1024  # 2MB
CHUNK_SIZE = 64  # bytes per write, adjust to EP size (512 for HS, 64 for FS)

def format_bytes(data):
    return ' '.join(f'{byte:02X}' for byte in data)

def find_device():
    dev = usb.core.find(idVendor=USB_VID, idProduct=USB_PID)
    if dev is None:
        raise RuntimeError(f'Device {USB_VID:#06x}:{USB_PID:#06x} not found')
    return dev

def get_bulk_endpoints(dev):
    cfg = dev.get_active_configuration()
    intf = cfg[(0, 0)]
    ep_out = usb.util.find_descriptor(intf, custom_match=lambda e:
        usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)
    ep_in = usb.util.find_descriptor(intf, custom_match=lambda e:
        usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)
    return ep_out, ep_in

def main():
    dev = find_device()
    print(f'Found device: {USB_VID:#06x}:{USB_PID:#06x}')

    # Detach kernel driver if needed
    if dev.is_kernel_driver_active(0):
        dev.detach_kernel_driver(0)

    dev.set_configuration()
    ep_out, ep_in = get_bulk_endpoints(dev)
    print(f'EP OUT: {ep_out.bEndpointAddress:#04x}, max packet: {ep_out.wMaxPacketSize}')

    # Generate 2MB of sequential data (0x00..0xFF repeating)
    data = bytes(i & 0xFF for i in range(TARGET_SIZE))
    print(f'First 10 bytes: {format_bytes(data[:10])}')
    print(f'Last 10 bytes:  {format_bytes(data[-10:])}')

    sent = 0
    start = time.time()

    print(f'Sending {TARGET_SIZE // 1024} KB in {CHUNK_SIZE}-byte chunks...')
    while sent < TARGET_SIZE:
        chunk = data[sent:sent + CHUNK_SIZE]
        written = ep_out.write(chunk, timeout=5000)
        sent += written
        if sent % (256 * 1024) == 0:
            elapsed = time.time() - start
            speed = sent / elapsed / 1024
            print(f'  Sent {sent // 1024} KB  ({speed:.1f} KB/s)')

    # Send ZLP to flush the last transfer when total is a multiple of CHUNK_SIZE
    if sent % CHUNK_SIZE == 0:
        ep_out.write(b'', timeout=5000)

    elapsed = time.time() - start
    print(f'\nDone. Sent {sent} bytes in {elapsed:.2f}s  ({sent / elapsed / 1024:.1f} KB/s)')

if __name__ == '__main__':
    main()
