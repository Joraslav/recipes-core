FIND_LIB(glaze)

add_library(_io_json STATIC
    Report.cpp
)

target_include_directories(_io_json
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/io/json>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/io/json>
)

target_link_libraries(_io_json PRIVATE
    _types
    glaze::glaze
)
