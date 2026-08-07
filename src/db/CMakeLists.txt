FIND_LIB(SQLiteCpp)

add_library(_db STATIC
 DBManager.cpp
)

target_include_directories(_db
 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/db>
  $<INSTALL_INTERFACE:include>
  $<INSTALL_INTERFACE:include/db>
)

target_link_libraries(_db PUBLIC
  SQLiteCpp
)
