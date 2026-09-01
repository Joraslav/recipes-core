FIND_LIB(glaze)

add_library(_api STATIC
    Json.cpp
)

target_include_directories(_api
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/api>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/api>
)

target_link_libraries(_api PUBLIC
    glaze::glaze
)