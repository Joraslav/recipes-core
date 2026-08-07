add_library(_io_cli STATIC
    Report.cpp
)

target_include_directories(_io_cli
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/io/console>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/io/console>
)

target_link_libraries(_io_cli PUBLIC
    _types
)
