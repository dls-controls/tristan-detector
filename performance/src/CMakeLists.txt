
include_directories(${Boost_INCLUDE_DIRS})

add_library(LATRD AsciiStackReader.cpp BufferBuilder.cpp DataWord.cpp)

add_executable (standard_performance standard_performance.cpp)
target_link_libraries (standard_performance LATRD ${Boost_LIBRARIES})

set(CMAKE_BUILD_TYPE Release)
