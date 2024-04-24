file(GLOB UI "wououi/*.c")

idf_component_register(SRCS "main.c" ${UI}
                    INCLUDE_DIRS "." "wououi/include/")
