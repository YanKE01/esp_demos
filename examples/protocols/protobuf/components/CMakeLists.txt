file(GLOB SRC_FILES *.c)
set(INC_FILES "include/")

idf_component_register(SRCS ${SRC_FILES}
                        INCLUDE_DIRS ${INC_FILES})