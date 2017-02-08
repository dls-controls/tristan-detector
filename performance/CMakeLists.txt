# Project name
project(latrd-performance)

# Require CMake version >=2.8
cmake_minimum_required(VERSION 2.8)

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(Boost_NO_BOOST_CMAKE "Disable search for boost-cmake (as it is broken on RHEL6)" ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Appends the cmake/modules path inside the MAKE_MODULE_PATH variable which stores the
# directories of additional CMake modules (ie. MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${pcvl-proc_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Find and add external packages required for application and test
# Additional components that can be included later if necessary:
#    program_options system filesystem date_time thread
find_package( Boost 1.41.0
	      REQUIRED
	      COMPONENTS program_options unit_test_framework )
if(Boost_FOUND)
  message(STATUS "  Boost include dirs: ${Boost_INCLUDE_DIRS}")
  message(STATUS "  Boost libs:         ${Boost_LIBRARIES}")
endif()

find_package( HDF5 1.8.9
	          REQUIRED C )
if(HDF5_FOUND)
  message(STATUS "  HDF5 include dirs: ${HDF5_INCLUDE_DIRS}")
  message(STATUS "  HDF5 definitions:  ${HDF5_DEFINITIONS}")
endif()

# Add include directory to include path
include_directories(Include)

# Add the source subdirectory
add_subdirectory(src)

# Add the HDF5 loader subdirectory
#add_subdirectory(H5_src)

# Add the test subdirectory
#add_subdirectory(test)

# Add the profiling subdirectory
#add_subdirectory(profile)

#if (TBB_ROOT)
#  message( "Searching for TBB root: ${TBB_ROOT}")
#endif()

#find_package( TBB )
#if (TBB_FOUND)
#  message( "Found TBB!!!")
#  add_subdirectory(percival_parallel)
#  add_subdirectory(parallel_test)
#else()
#  message( "No TBB package found. Not building percival_parallel/" )
#endif()




