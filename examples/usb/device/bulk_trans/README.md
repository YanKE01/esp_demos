# USB Bulk Trans

This example demonstrates how to implement bulk transfer using the USB interface of the ESP32-S3. The example only shows the read and echo functionality.

The Linux driver in the linux_driver folder is the host driver for the Linux platform. It will actively initiate a USB transfer with a packet size of 640 bytes. However, since the ESP32-S3 endpoint size is only 64 bytes, the data needs to be encoded to ensure that the ESP32-S3 can correctly parse all the data.

To compile and test, you need to first compile the driver under ``linux_driver`` and ``insmod`` it into the kernel, then insert the ESP32-S3 chip with the downloaded ``bulk_trans`` code.
