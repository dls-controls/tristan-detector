
include_directories(${Boost_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS})

add_library(LATRD AsciiStackReader.cpp BufferBuilder.cpp DataWord.cpp)

add_executable (standard_performance standard_performance.cpp)
target_link_libraries (standard_performance LATRD ${Boost_LIBRARIES})

add_executable (tbb_performance pipeline_performance.cpp)
target_link_libraries (tbb_performance LATRD ${Boost_LIBRARIES} ${TBB_LIBRARIES})

set(CMAKE_BUILD_TYPE Release)
