Installation
============

Installing From Source
----------------------

Installation from source requires several tools and external packages to be available as 
well as the building and installation of three Odin and Eiger specific software packages.
This guide assumes the the installation is taking place on a CentOS 7 (or RHEL 7) operating
system.  The source can also be built for other operating systems;
any operating system specific package installation commands should simply be replaced with
the equivalent command for the chosen operating system.

Prerequisites
-------------

The Odin and Tristan software requires standard C, C++ and python development tools to be 
present.  The following libraries and tools are also required:

* boost (add link)
* cmake (add link)
* log4cxx (add link)
* ZeroMQ (add link)
* pcap (add link)
* Cython (add link)
* Python Pip (add link)
* Python Virtual Environment (add link)

All of the above packages can be installed from the default package manager Yum (add link) 
with the following commands::


    yum install -y epel-release
    yum groupinstall -y 'Development Tools'
    yum install -y boost boost-devel
    yum install -y cmake
    yum install -y log4cxx-devel
    yum install -y zeromq3-devel
    yum install -y libpcap-devel
    yum install -y python2-pip
    yum install -y python-virtualenv
    yum install -y Cython

For all remaining sections of this installation guide it is assumed that source files are 
going to be located in the directory::

    /home/tristan/

This directory will be referred to as the $SRC directory::

    export SRC=/home/tristan


Install Python Virtual Environment
----------------------------------

The current version of Tristan control software runs on Python version 2.7.  The easiest
method of setting up and installing an environment is to use pip.
Start by creating the virtual environment::

    cd $SRC
    virtualenv venv27


Now activate the environment for use::

    source $SRC/venv27/bin/activate


Update the version of pip and the virtual environment::

    pip install --upgrade pip
    pip install --upgrade virtualenv


This virtual environment will be used to install all of the required modules for the
detector software.


Installing Odin-Control
-----------------------

Odin-Control contains the control server which will act as the central point of control
for all other parts of the software stack.  This module contains only Python.  Start by
downloading the odin-control module::

    cd $SRC
    wget -O odin-control.zip https://github.com/dls-controls/odin-control/archive/external-build.zip
    unzip odin-control.zip 
    mv odin-control-external-build/ odin-control


If preferred the module can be cloned from github using git::

    cd $SRC
    git clone https://github.com/dls-controls/odin-control.git
    cd odin-control
    git checkout external-build


Python Installation
*******************

Activate the Python virtual environment (if not already activated) and install the
requirements and then the module itself::

    source $SRC/venv27/bin/activate
    cd $SRC/odin-control/
    pip install -r requirements.txt 
    python ./setup.py install


At this stage it is possible to test that the installation was successful by requesting
the command line help of the Odin control server application::

    odin_server --help



Installing Odin-Data
--------------------

Odin-Data contains both Python and C++ software.  The main data acquisition base software
is contained (FrameReceiver and FrameProcessor applications) as well as the necessary 
Python control software (Python adapters that are loaded into the Odin control server)
to control the data acquisition applications.  Finally, this module contains the application
known as the MetaWriter which Tristan also requires.
At the current time of writing a development branch is required to run the Tristan software.

To start the installation, download the odin-data module::

    cd $SRC
    wget -O odin-data.zip https://github.com/dls-controls/odin-data/archive/dev_branch.zip
    unzip odin-data.zip 
    mv odin-data-dev_branch odin-data
    cd odin-data


If preferred the module can be cloned from github using git::

    cd $SRC
    git clone https://github.com/dls-controls/odin-data.git
    cd odin-data
    git checkout dev_branch

Python Installation
*******************

Activate the Python virtual environment (if not already activated) and install the
requirements and then the module itself::

    source $SRC/venv27/bin/activate
    cd $SRC/odin-data/tools/python
    pip install -r requirements.txt 
    python ./setup.py install

At this stage it is possible to test that the Python installation was successful by
requesting the command line help of the MetaWriter application::

    meta_writer --help


C++ Installation
****************


TDB: C++ Installation


Installing Tristan-Detector
---------------------------

Tristan-Detector contains both Python and C++ software.  This module contains the control
and data acquisition libraries that are specific to the detector, and these are loaded 
into the control and data acquisition applications from the other modules.

To start the installation, download the odin-data module::

    cd $SRC
    wget -O tristan-detector.zip https://github.com/dls-controls/tristan-detector/archive/meta.zip
    unzip tristan-detector.zip 
    mv tristan-detector-meta tristan-detector
    cd tristan-detector


If preferred the module can be cloned from github using git::

    cd $SRC
    git clone https://github.com/dls-controls/tristan-detector.git
    cd tristan-detector
    git checkout meta


Python Installation
*******************

Activate the Python virtual environment (if not already activated) and install the
requirements and then the module itself::

    source $SRC/venv27/bin/activate
    cd $SRC/tristan-detector/control
    pip install -r control_requirements.txt
    python ./setup.py install


At this stage it is possible to test that the Python installation was successful by
requesting the command line help of the simulator application::

    tristan_simulator --help


C++ Installation
****************


TDB: C++ Installation

