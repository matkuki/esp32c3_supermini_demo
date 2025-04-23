idf_component_register(
    SRCS "i2c_controller.c" "i2c_chipcap2.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver
)