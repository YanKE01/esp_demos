# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Create the spiffs directory and a placeholder file during the configuration phase to ensure EMBED_FILES can locate the file
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/spiffs)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/spiffs/merged-binary.bin)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/spiffs/merged-binary.bin "")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(build_two_target)

# Add a custom target to first build example_c6, generate merged-binary.bin, and move it to the spiffs directory
# Use add_custom_command to generate the file and ensure dependencies are set correctly
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/spiffs/merged-binary.bin
    COMMAND bash -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/example_c6 && idf.py set-target esp32c6 && idf.py build && idf.py merge-bin"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/spiffs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/example_c6/build/merged-binary.bin ${CMAKE_CURRENT_SOURCE_DIR}/spiffs/merged-binary.bin
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building example_c6 with target esp32c6 and copying merged-binary.bin to spiffs"
    VERBATIM
)

add_custom_target(build_example_c6 ALL
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/spiffs/merged-binary.bin
)
