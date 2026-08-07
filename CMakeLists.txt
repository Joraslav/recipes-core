cmake_minimum_required(VERSION 4.1.0)
project(Recipes VERSION 0.1.0 LANGUAGES CXX)

include(GoogleTest)
include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
  -Wall
  -Wextra
  -pedantic
  -Wno-unused-parameter
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/functions.cmake)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/db)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/io)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/types)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/app)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)

if(BUILD_TESTING)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
 _app
)
