file(GLOB APP_LVGL_SCRS "app_lvgl/*.c")

idf_component_register(SRCS "main.c" "hal_camera/hal_camera.c" ${APP_LVGL_SCRS}
                    INCLUDE_DIRS "." "hal_camera/include/" "app_lvgl/include/")
