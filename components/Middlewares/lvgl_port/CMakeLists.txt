# components/Middlewares/lvgl_port/CMakeLists.txt
idf_component_register(
    SRCS 
        "lv_port_disp.c"
        "lv_tick.c"
        "lv_port_indev.c"
    INCLUDE_DIRS "."
    REQUIRES 
        lvgl
        esp_timer
        esp_lcd
        esp_lcd_touch 
        esp_lcd_nt35510
        BSP
)
