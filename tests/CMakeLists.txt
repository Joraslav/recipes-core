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

ADD_GTEST_TARGET(TestIOArgs
	SOURCES
		TestIOArgs.cpp
	LINK_LIBS
		_io
		_types
)
