file(GLOB APP_USB_SRCS "./app_usb/*.c")

idf_component_register(SRCS "main.c" ${APP_USB_SRCS}
                    INCLUDE_DIRS "." "app_usb/include/")
