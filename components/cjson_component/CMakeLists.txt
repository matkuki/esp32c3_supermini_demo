idf_component_register(
    SRCS "cjson_component.c" ""
    INCLUDE_DIRS "."
    REQUIRES driver i2c_components uart_component
)