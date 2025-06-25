DRIVER_NAME := esp_usb_test
KERNEL_SOURCE_DIR ?= /lib/modules/`uname -r`/build

obj-m := $(DRIVER_NAME).o

DRIVER_FILES := usb.o
CFLAGS_main.o = -O0

$(DRIVER_NAME)-objs:= $(DRIVER_FILES)

modules:
	$(MAKE) -C $(KERNEL_SOURCE_DIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNEL_SOURCE_DIR) M=$(PWD) modules_install

install: modules_install

clean:
	$(MAKE) -C $(KERNEL_SOURCE_DIR) M=$(PWD) clean
