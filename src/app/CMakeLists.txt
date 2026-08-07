add_library(_app STATIC
    App.cpp
)

target_include_directories(_app
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/app>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/app>
)

target_link_libraries(_app PUBLIC
    _db
    _io
    _types
)