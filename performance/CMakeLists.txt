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

# Set up the boost libraries
set(Boost_DIR /dls_sw/prod/tools/RHEL6-x86_64/boost/1-48-0/prefix)
set(Boost_INCLUDE_DIRS ${Boost_DIR}/include)
set(Boost_LIBRARIES ${Boost_DIR}/lib/libboost_program_options.so;
					${Boost_DIR}/lib/libboost_unit_test_framework.so;
					${Boost_DIR}/lib/libboost_thread.so;
					${Boost_DIR}/lib/libboost_system.so;)
message(STATUS "  Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "  Boost libs:         ${Boost_LIBRARIES}")

# Set up the Intel TBB libraries
set(TBB_DIR /dls_sw/prod/tools/RHEL6-x86_64/tbb/4-3-6/prefix)
set(TBB_INCLUDE_DIRS ${TBB_DIR}/include)
set(TBB_LIBRARIES ${TBB_DIR}/lib/libtbb.so;
				  ${TBB_DIR}/lib/libtbbmalloc.so;
				  ${TBB_DIR}/lib/libtbbmalloc_proxy.so;)
message(STATUS "  TBB include dirs: ${TBB_INCLUDE_DIRS}")
message(STATUS "  TBB libs:         ${TBB_LIBRARIES}")


# Set up the HDF5 libraries
set(HDF5_DIR /dls_sw/prod/tools/RHEL6-x86_64/hdf5/1-10-0/prefix)
set(HDF5_INCLUDE_DIRS ${HDF5_DIR}/include)
set(HDF5_LIBRARIES ${HDF5_DIR}/lib/libhdf5.so;
				   ${HDF5_DIR}/lib/libhdf5_hl.so;)
message(STATUS "  HDF5 include dirs: ${HDF5_INCLUDE_DIRS}")
message(STATUS "  HDF5 libs:         ${HDF5_LIBRARIES}")


#find_package( HDF5 1.8.9
#	          REQUIRED C )
#if(HDF5_FOUND)
#  message(STATUS "  HDF5 include dirs: ${HDF5_INCLUDE_DIRS}")
#  message(STATUS "  HDF5 definitions:  ${HDF5_DEFINITIONS}")
#endif()

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




