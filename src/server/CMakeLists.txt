FIND_LIB(Boost)
FIND_LIB(OpenSSL)

add_library(_server STATIC
  Router.cpp
  Server.cpp
)

target_include_directories(_server
 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/server>
  $<INSTALL_INTERFACE:include>
  $<INSTALL_INTERFACE:include/server>
)

target_link_libraries(_server PUBLIC
  _api
  _app
  Boost::boost
  Boost::cobalt
  Boost::cobalt_io_ssl
  OpenSSL::SSL
  OpenSSL::Crypto
)
