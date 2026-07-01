FIND_LIB(SQLiteCpp)

add_library(_db STATIC
 DBManager.cpp
)

target_include_directories(_db
 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(_db PUBLIC
  SQLiteCpp
)
