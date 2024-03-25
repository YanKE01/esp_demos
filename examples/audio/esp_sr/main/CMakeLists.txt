set(SRC_FILES "app/app_sr.c" "hal/hal_i2s.c" "app/app_led.c")
set(INC_DIRS "app/include" "hal/include/")

idf_component_register(SRCS "main.c" ${SRC_FILES}
                    INCLUDE_DIRS "." ${INC_DIRS}
                    )
