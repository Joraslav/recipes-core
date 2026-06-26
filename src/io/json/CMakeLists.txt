FIND_LIB(glaze)

add_library(_io_json STATIC
    Report.cpp
)

target_include_directories(_io_json
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(_io_json PRIVATE
    _types
    glaze::glaze
)
