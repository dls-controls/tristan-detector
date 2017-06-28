
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

include_directories(${FRAMERECEIVER_DIR}/include ${ODINDATA_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

# Build list of test source files from current dir
#file(GLOB TEST_SOURCES *.cpp)

# Build list of main project source files from src dir but exclude application main
#file(GLOB APP_SOURCES ${FRAMERECEIVER_DIR}/src/*.cpp)
#file(GLOB LIB_SOURCES ${FRAMERECEIVER_DIR}/src/*Lib.cpp)
#list(REMOVE_ITEM APP_SOURCES ${LIB_SOURCES})

# Add test and project source files to executable
#add_executable(LATRDDecoderTest ${TEST_SOURCES} ${APP_SOURCES})

#if ( ${CMAKE_SYSTEM_NAME} MATCHES Linux )
# librt required for timing functions
#find_library(REALTIME_LIBRARY 
#		NAMES rt)
#target_link_libraries( LATRDDecoderTest ${REALTIME_LIBRARY} )
#endif()

# Define libraries to link against
#target_link_libraries(LATRDDecoderTest 
#		${ODINDATA_LIBRARIES} 
#		${Boost_LIBRARIES} 
#		${LOG4CXX_LIBRARIES} 
#		${ZEROMQ_LIBRARIES} )
