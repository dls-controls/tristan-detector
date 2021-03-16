.. tristan-detector documentation master file, created by
   sphinx-quickstart on Fri Mar 12 11:25:37 2021.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Tristan Detector
================

The Tristan Detector is a modular detector based on the Timepix3 ASIC from the Medipix3
Collaboration. While Tristan can be considered as an imaging detector, it does not 
present conventional image frames, rather it provides an event driven stream of four 
dimensional sparse data.

This module is a software solution for the control of and acquisition of data from a 
Tristan Detector.  This module is built on top of the detector framework `odin-detector`_
which has been created for ease of integration, scalability, and highly efficient data
acquisition.  The module does not require a particular control middleware to act as a 
client, providing instead a RESTful API over HTTP.

A web based graphical user interface (GUI) is supplied with the module as a default client
to allow immediate testing once the software has been built.

There is also a separate software repository `ADOdin`_ that contains an EPICS client 
application which can be used to control this software stack.

Documentation
-------------

The following documentation is available for Tristan Detector:

* The :doc:`introduction <introduction>` section provides an overview of the various sotware components and a guide to the odin-detector framework.
* The :doc:`installation <installation>` instructions explain in detail the steps necessary to install the software module on a linux OS.
* The :doc:`setting up <setup>` guide demonstrates how to setup and run a Tristan instance using the provided simulator, and then swapping the simulator for the detector hardware.
* The :doc:`user guide <userguide>` demonstrates how to operate the detector with the software.


Other
-----

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   introduction
   installation
   setup
   userguide

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

.. _odin-detector:
    https://github.com/odin-detector

.. _ADOdin:
    https://github.com/dls-controls/ADOdin
