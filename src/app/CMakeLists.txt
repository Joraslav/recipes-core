add_library(_app STATIC
    App.cpp
)

target_include_directories(_app
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(_app PUBLIC
    _db
    _io
    _types
)