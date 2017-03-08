# LATRD
Large Area Time Resolved Detector

Build Instructions
==================

	mkdir build
	cd build
	cmake -DBoost_NO_BOOST_CMAKE=ON \
	      -DLOG4CXX_ROOT_DIR=/dls_sw/prod/tools/RHEL6-x86_64/log4cxx/0-10-0/prefix \
	      -DZEROMQ_ROOTDIR=/dls_sw/prod/tools/RHEL6-x86_64/zeromq/3-2-4/prefix \
	      -DODINDATA_ROOT_DIR=/home/gnx91527/work/odin-data/build \
	      -DHDF5_ROOT=/dls_sw/prod/tools/RHEL6-x86_64/hdf5/1-10-0/prefix \
	      ..