"""Large Area Time Resolved Detector (Tristan) setuptools based setup module.
Created on 12 January 2017
@author: Alan Greer
"""

from setuptools import setup, find_packages
# To use a consistent encoding
from codecs import open
from os import path
import versioneer

rootdir = path.abspath(path.dirname(__file__))

# Get the long description from the relevant file
with open(path.join(rootdir, '../README.md'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name='tristan-detector',

    version=versioneer.get_version(),

    description='A Python based odin-control interface library to the Tristan detector',
    long_description=long_description,

    # The project's main homepage.
    url='https://github.com/dls-controls/LATRD',

    # Author details
    author='Alan Greer',
    author_email='alan.greer@diamond.ac.uk',

    # Choose your license
    license='Apache Software License, Version 2.0',

    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        # How mature is this project? Common values are
        #   3 - Alpha
        #   4 - Beta
        #   5 - Production/Stable
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Science/Research',
        'Topic :: Scientific/Engineering',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
    ],
    keywords='Timepix Tristan Detector Science',

    # Specify the packages that this project provides (using find_packages() for automation)
    packages=find_packages(exclude=['docs', 'sandbox', 'tests*']),

    # run-time dependencies here. These will be installed by pip when the project is installed.
    #install_requires=['numpy==1.11.1', 'h5py==2.6.0', 'future==0.15.2', 'enum34==1.1.6', 'npyscreen==4.10.5',
    # 'pyzmq==15.3.0'],
    install_requires=['odin-control', 'odin-data', 'configparser', 'python-dateutil', 'enum34'],

    # Additional groups of dependencies (e.g. development dependencies). 
    # You can install these using the following syntax, for example:
    # $ pip install -e .[dev,test]
    extras_require={
        'test': ['nose>=1.3', 'coverage', 'mock'],
    },

    # Data files included in the packages
    #package_data={
    #    'sample': ['package_data.dat'],
    #},

    # Although 'package_data' is the preferred approach, in some case you may
    # need to place data files outside of your packages. See:
    # In this case, 'data_file' will be installed into '<sys.prefix>/my_data'
    #data_files=[('my_data', ['data/data_file'])],

    # To provide executable scripts, use entry points in preference to the
    # "scripts" keyword. Entry points provide cross-platform support and allow
    # pip to create the appropriate form of executable for the target platform.
    entry_points={
        'console_scripts': [
            'tristan_odin=odin.server:main',
            'tristan_simulator=latrd.detector.control_simulator:main',
            'test-control-interface=latrd.detector.test_control_interface:main',
        ],
    },
)

