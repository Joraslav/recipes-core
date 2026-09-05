FIND_LIB(GTest)

ADD_GTEST_TARGET(TestDBManager
    SOURCES
    TestDBManager.cpp
    LINK_LIBS
    _db
    _types
)

ADD_GTEST_TARGET(TestKitchenTypes
    SOURCES
    TestKitchenTypes.cpp
    LINK_LIBS
    _types
)

ADD_GTEST_TARGET(TestIOConsole
    SOURCES
    TestIOConsole.cpp
    LINK_LIBS
    _io
    _types
)

ADD_GTEST_TARGET(TestIOJson
    SOURCES
    TestIOJson.cpp
    LINK_LIBS
    _io
    _types
)

ADD_GTEST_TARGET(TestIOYaml
    SOURCES
    TestIOYaml.cpp
    LINK_LIBS
    _io
    _types
)

ADD_GTEST_TARGET(TestIOArgs
    SOURCES
    TestIOArgs.cpp
    LINK_LIBS
    _io
    _types
)

ADD_GTEST_TARGET(TestApi
    SOURCES
    TestApi.cpp
    LINK_LIBS
    _api
)

ADD_GTEST_TARGET(TestRecipeService
    SOURCES
    TestRecipeService.cpp
    LINK_LIBS
    _app
)

ADD_GTEST_TARGET(TestSqliteRecipeRepository
    SOURCES
    TestSqliteRecipeRepository.cpp
    LINK_LIBS
    _app
)

ADD_GTEST_TARGET(TestDatabaseExecutor
    SOURCES
    TestDatabaseExecutor.cpp
    LINK_LIBS
    _app
)

ADD_GTEST_TARGET(TestHttpRouter
    SOURCES
    TestHttpRouter.cpp
    LINK_LIBS
    _server
)

ADD_GTEST_TARGET(TestServer
    SOURCES
    TestServer.cpp
    LINK_LIBS
    _server
)

ADD_GTEST_TARGET(TestServerConfig
    SOURCES
    TestServerConfig.cpp
    LINK_LIBS
    _config
)
