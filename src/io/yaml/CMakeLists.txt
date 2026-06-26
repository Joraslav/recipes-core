FIND_LIB(glaze)

add_library(_io_yaml STATIC
	Report.cpp
)

target_include_directories(_io_yaml
    PUBLIC
	$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(_io_yaml PUBLIC
    _types
    glaze::glaze
)
