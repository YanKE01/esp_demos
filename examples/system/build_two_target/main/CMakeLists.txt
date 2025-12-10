idf_component_register(SRCS "build_two_target.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "../spiffs/merged-binary.bin")

# Ensure that merged-binary.bin has been generated before building the main component
# Add a dependency to make sure example_c6 is built before building the main component
if(TARGET build_example_c6)
    add_dependencies(${COMPONENT_LIB} build_example_c6)
endif()
