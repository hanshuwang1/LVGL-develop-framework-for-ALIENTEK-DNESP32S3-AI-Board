# components/Middlewares/gui/CMakeLists.txt
# file(GLOB_RECURSE GUI_SRCS "screens/*.c" "widgets/*.c")

set(ui_src_dirs
            Button
            )

set(ui_include_dirs
            Button
            )

set(requires
            lvgl
            lvgl_port
            )

idf_component_register(SRC_DIRS ${ui_src_dirs} INCLUDE_DIRS ${ui_include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
