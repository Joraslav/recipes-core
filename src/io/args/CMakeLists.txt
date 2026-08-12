add_library(_io_args STATIC
	Args.cpp
)

target_include_directories(_io_args
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/io/args>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/io/args>
)

target_compile_definitions(_io_args
    PUBLIC
    RECIPES_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(_io_args PUBLIC
    _types
)
