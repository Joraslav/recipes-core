add_library(_io_cli STATIC
    Report.cpp
)

target_include_directories(_io_cli
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(_io_cli PUBLIC
    _types
)
