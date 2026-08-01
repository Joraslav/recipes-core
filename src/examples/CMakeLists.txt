add_executable(AddToDb
  AddToDb.cpp
)

target_include_directories(AddToDb
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)

target_link_libraries(AddToDb PRIVATE
  _db
  _io
  _types
)
