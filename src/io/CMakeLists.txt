add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/console)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/json)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/yaml)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/args)

add_library(_io STATIC
    IO.cpp
)

target_include_directories(_io
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/io>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/io>
)

target_link_libraries(_io PUBLIC
    _io_cli
    _io_json
    _io_yaml
    _io_args
)
