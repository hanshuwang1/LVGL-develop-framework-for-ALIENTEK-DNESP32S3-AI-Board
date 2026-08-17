set(src_dirs
            LED
            IIC
            XL9555
            LCD
            TOUCH
            QMA6100P
            )

set(include_dirs
            IIC
            LED
            XL9555
            LCD
            TOUCH
            QMA6100P
            )

set(requires
            esp_driver_gpio
            esp_driver_i2c
            esp_lcd_nt35510
            esp_lcd_touch
            esp_lcd_touch_gt911
            lvgl
            )

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
