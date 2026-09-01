FIND_LIB(glaze)

add_library(_config STATIC
    ServerConfig.cpp
)

target_include_directories(_config
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/config>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/config>
)

target_link_libraries(_config PUBLIC
    glaze::glaze
)