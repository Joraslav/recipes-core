FIND_LIB(glaze)

add_library(_io_yaml STATIC
	Report.cpp
)

target_include_directories(_io_yaml
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/io/yaml>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/io/yaml>
)

target_link_libraries(_io_yaml PUBLIC
    _types
    glaze::glaze
)
