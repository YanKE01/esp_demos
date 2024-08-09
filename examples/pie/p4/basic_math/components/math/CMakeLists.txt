file(GLOB MATH_SRCS "*.c")

idf_component_register(SRCS ${MATH_SRCS}
                    INCLUDE_DIRS "include"
                    )
