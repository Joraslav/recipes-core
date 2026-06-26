FIND_LIB(SQLiteCpp)

add_library(_types INTERFACE)

target_include_directories(_types
 INTERFACE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(_types INTERFACE
 SQLiteCpp
)
