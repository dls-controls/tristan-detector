
include_directories(${Boost_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS})

add_library(LATRD AsciiStackReader.cpp BufferBuilder.cpp DataWord.cpp DataBlock.cpp FileWriter.cpp)

add_executable (simple_writing_performance_test simple_writing_performance_test.cpp)
target_link_libraries (simple_writing_performance_test ${Boost_LIBRARIES} ${HDF5_LIBRARIES})

add_executable (standard_performance standard_performance.cpp)
target_link_libraries (standard_performance LATRD ${Boost_LIBRARIES} ${HDF5_LIBRARIES})

add_executable (tbb_performance pipeline_performance.cpp)
target_link_libraries (tbb_performance LATRD ${Boost_LIBRARIES} ${TBB_LIBRARIES} ${HDF5_LIBRARIES})

set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Debug)
