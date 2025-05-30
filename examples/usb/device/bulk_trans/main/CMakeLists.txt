idf_component_register(SRC_DIRS "."
                    INCLUDE_DIRS "." "tusb")

# Determine whether tinyusb is fetched from component registry or from local path
idf_build_get_property(build_components BUILD_COMPONENTS)
if(tinyusb IN_LIST build_components)
    set(tinyusb_name tinyusb) # Local component
else()
    set(tinyusb_name espressif__tinyusb) # Managed component
endif()

idf_component_get_property(tusb_lib ${tinyusb_name} COMPONENT_LIB)
target_include_directories(${tusb_lib} PUBLIC "${PROJECT_DIR}/main/tusb")
target_sources(${tusb_lib} PUBLIC "${PROJECT_DIR}/main/tusb/usb_descriptors.c")

cmake_policy(SET CMP0079 NEW)
target_link_libraries(${tusb_lib} PRIVATE ${COMPONENT_LIB})
