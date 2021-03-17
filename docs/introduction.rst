Introduction
============

Tristan Detector
----------------

The Tristan Detector is a modular detector based on the Timepix3 ASIC from the Medipix3
Collaboration. While Tristan can be considered as an imaging detector, it does not 
present conventional image frames, rather it provides an event driven stream of four 
dimensional sparse data.

The detector hardware can be controlled by using a JSON format message protocol over a
ZeroMQ based TCP message socket.  The detector produces UDP packets containing the event 
data, including timestamps, IDs and ToT information.  As well as the event data the UDP 
packets can contain additional control and status information.

A separate monitor (live view) image can be retrieved from the detector using another 
ZeroMQ channel.  The monitor image is transferred as a combination of JSON description 
header plus raw byte data.  The live view frames are not compressed and are available 
at a much reduced rate.

This Tristan Detector software stack provides applications for the following:

* Controlling the detector parameters and acquisition.
* Monitoring the detector status.
* Receiving time resolved data packets vi UDP and processing them into timestamped events.
* Manipulating and writing the event data to disk (using HDF5).
* Writing associated meta data to disk (using HDF5).

The ability to alter the destination of individual modules of a Tristan detector provides 
scalability of the software stack.  As data rates get higher more writing nodes can be introduced 
to the system and these nodes can reside on different physical hardware.  More details of the 
scalabililty are provided in the deployment section below.

All of the applications mentioned above are handled through a single point of control (the 
Odin control server) and this provides a simple RESTful API to interact with the whole stack.
The Odin control server also serves a set of static HTML pages by default which provide a 
web based Graphical User Interface (GUI) that can be used without the need to integrate the 
software stack into another framework; all that is required is a standard browser.

Odin Detector
-------------
Devices consisting of multiple individual parts can lead
to complications in the control layer trying to get operate
them together in unity. The Odin software framework is
designed specifically for this modular architecture by mirroring 
the structure within its internal processes. The data
acquisition modules have the perspective of being one of
many nodes built into the core of their logic. This makes it
straightforward to operate multiple file writers on different
server nodes working together to write a single acquisition
to disk, all managed by a single point of control. It also
means that the difference in the data acquisition stack of a
1M system and a 10M system can be as little as duplicating a
few processes and modifying the configuration of the central
controller. Given the collaborative nature of the detector
development, the software framework has been designed to be 
generic, allowing its integration with control systems used
at different sites.

The figure below shows the OdinControl Server architecture for
a 10M Tristan Odin system.


    Figure 1. OdinControl Server architecture for a 10M Tristan Odin system.

Figure 1 demonstrates the generic RESTful interface that Odin
provides.  The control server can be accessed by four different 
methods:

  * ADOdin - This is an EPICS areaDetector implementation that can interface to Odin detector systems.
  * Web Browswer - Simply pointing a standard web browser at the Odin control server will open a web based GUI for control and monitoring.
  * Python Script - Using the requests module (or equivalent) HTTP requests can be scripted to control and monitor the system.
  * Command Line - The standard Linux tool curl can be used to send HTTP requests for control and monitoring.

Odin comprises two parts; Odin Control, the central control application, 
and Odin Data, a data acquisition stack, both of which
can be used independently of each other as well as in conjunction.
These modules are described, separately, below.

Odin Control
------------

OdinControl is a HTTP based server host providing a
framework that device-specific adapters can be implemented
for to interface to the control channel of a device. The archi-
tecture of OdinControl for the Tristan use case is shown
in Fig. 2. Adapters can be loaded in an Odin Server instance,
which then provides a REST API that can be operated us-
ing just a web page providing the appropriate GETs and
PUTs, corresponding to the attributes and methods in the
adapter API. See Fig. 3 for an example web page for Tristan. 
This can be extended to a RESTful client library that
can be integrated into a higher level control system such as
EPICS. Once an Odin Server instance is running with a set
of adapters loaded, a parameter tree is created in the API
defining the different devices, duplicates of the same devices
and finally the endpoints for those devices, producing logical 
paths to the parameters and methods of a collection of
separate systems. With OdinControl and a device adapter, a
control system agnostic, consistent API is created that can be
used in a wide range of applications. OdinControl provides
a simple Python API, enabling rapid development of device
adapters. Because of the generic architecture of OdinControl 
it does not need a tight coupling to OdinData; OdinData is also
interfaced via adapters, just like the detector control, to
provide a REST API for a set of methods. This keeps the two
parts of Odin entirely separate, achieving a good software
design with loose coupling and high cohesion.

Odin Data
---------

OdinData gathers incoming frames from a data stream
and writes them to disk as quickly as possible. It has a
modular architecture making it simple to add functionality
and extend its use for new detectors. The function of the
software itself is relatively simple, allowing a higher-level 
supervisory control process to do the complex logic defined by
each experimental situation and exchanging simple configuration 
messages to perform specific operations. This makes
it easy for the control system to operate separate systems
cooperatively.
OdinData consists of two separate processes. These are
the FrameReceiver (FR) and the FrameProcessor (FP). The
FR is able to collect data packets on various input channel
types, for example UDP and ZeroMQ, construct data
frames and add some useful meta data to the packet header
before passing it on to the FP through a shared memory
interface. The FP can then grab the frame, construct data
chunks in the correct format and write them to disk. The
two separate processes communicate via inter-process 
communication (IPC) messages over two ZMQ channels. When
the FR places a frame into shared memory, it sends a message 
over the ready channel, the FP consumes the frame and
once it is finished passes a message over the release channel
allowing the FR to re-use the frame memory. The use and
re-use of shared memory reduces the copying of large data
blobs and increases data throughput. This logic is shown
visually in Fig. 4.
The overall concept is to allow a scalable, parallel data
acquisition stack writing data to a individual files in a shared
network location. This allows fine tuning of the process
nodes for a given detector system, based on the image size
and frame rate, to make sure the beamline has the capacity to
carry out its experiments and minimise the data acquisition
bottleneck.

Plugins
*******

OdinData is extensible by the implementation of plugins.
The Tristan detector has a module with two plugins built
against the OdinData library. One for the FR and one for
the FP. These provide the implementation of a decoder of
the raw frame data as well as the processing required to
define the data structure written to disk. The
implementation of any other detector would simply require
the two plugins to be replaced with equivalents, to process
the output data stream; the surrounding logic would remain
exactly the same.

API
***

OdinData provides a python library with simple methods
for initialising, configuring and retrieving status from the
FP and FR processes at runtime. These can be integrated
with a wider control system, but can also be used directly in
a simple python script or interactively from a python shell.
This is how OdinData integrates with OdinControl; there is
no special access granted, the interface is generic allowing
it to be integrated with other control systems.

HDF5 Features
*************

To take advantage of the high data rates of modern de-
tectors, OdinData seeks to write data to disk quickly with
minimal processing overhead. To achieve this, the built-in
FileWriterPlugin employs some of the latest features of the
HDF5 library.
The Virtual Dataset (VDS) enables the file writing to
be delegated to a number of independent, parallel processes,
because the data can all be presented as a single file at the
end of an acquisition using VDS to link to the raw datasets.
Secondly, with Single Writer Multiple Reader (SWMR)
functionality, datasets are readable throughout the acquisition 
and live processing can be carried out while frames are
still being captured, greatly reducing the overall time to 
produce useful data. Though the real benefit comes when these
two features are combined. A VDS can be created anytime
before, during or after and acquisition, independent of when
the raw datasets are created. Then, as soon as the parallel
writers begin writing to each raw file, the data appears in
the VDS as if the processes were all writing to the same file
and can be accessed by data analysis processes in exactly
the same way.
A more straightforward improvement in the form of a data
throughput increase is found by the use of Direct Chunk
Write. With a little extra effort in the formatting of the
data chunk, this allows the writer to skip the processing
pipeline that comes with the standard write method and
write a chunk straight to disk as provided. This reduces the
processing required and limits data copying.
