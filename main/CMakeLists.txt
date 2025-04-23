set(srcs "esp32c3_supermini_demo.c")

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
)
