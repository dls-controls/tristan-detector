# LATRD
Large Area Time Resolved Detector

Python Control Software Instructions
==========================================

System dependencies:

    Python (2.7)
    pip - python package manager
    ZeroMQ (development package)

Building with setuptools will attempt to use pip to download and install dependencies locally first. The python dependencies are listed in control_requirements.txt

To install the example control script and simulator

    virtualenv -p <path to python2.7> --no-site-packages venv27
    source venv27/bin/activate
    pip install --upgrade pip
    pip install --upgrade virtualenv
    pip install -r control_requirements.txt
    python setup.py install

To execute the simulator

    source venv27/bin/activate
    latrd-simulator

and to run the example test client script

    source venv27/bin/activate
    test-control-interface



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