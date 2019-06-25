==================================
FogLAMP Tiggered RMS Filter plugin
==================================

Simple readings data transformation plugin that calculates the RMS value
of data points over a variable data range. The duratoin of the RMS
sample is defined by a trigger asset and datapoint. When the trigger
value crosses the zero value, from negiative to positive, then an RMS
calculation is done for each of the data points and readings the filter
is processing.

It may optionally also include peak to peak measurements (i.e. the
maximum swing) within the same data period as the RMS value is calculated.

Note, peak values may be less than individual values of the input if the
asset value does not fall to or below zero. Where a data value swings
between negative and positive values then the peak value will be greater
than the maximum value in the data stream. For example if the minimum value
of a data point in the sample set is 0.3 and the maximum is 3.4 then the peak
value will be 3.1. If the maximum value is 2.4 and the minimum is zero then
the peak will be 2.4. If the maximum value is 1.7 and the minimum is -0.5
then the peak value will be 2.2.

The user may also choose to include or not the raw data that is used to
calculate the RMS values via a switch in the configuration.

Where a datastream has multiple assets within it the RMS filter may
be limited to work only on those assets whose name matches a regular
expression given in the configuration of the filter. The default for
this expression is .*, i.e. all assets are processed.

Runtime configuration
=====================

A number of configuration options exist:

triggerAsset
  The name of the asset that triggers the RMS calculation

triggerDatapoint
  The name of the datapoint within the trigger asset that triggers the
  RMS calculation

assetName
  The asset name to use to output the RMS values. "%a" will be replaced
  with the original asset name.

rawData 
  A switch to include the raw input data in the output

peak
  A switch to include peak to peak measurements for the same data set
  as the RMS measurement

match
  A  regular expression to limit the asset names on which this filter
  operations

Build
-----
To build FogLAMP "RMS" C++ filter plugin:

.. code-block:: console

  $ mkdir build
  $ cd build
  $ cmake ..
  $ make

- By default the FogLAMP develop package header files and libraries
  are expected to be located in /usr/include/foglamp and /usr/lib/foglamp
- If **FOGLAMP_ROOT** env var is set and no -D options are set,
  the header files and libraries paths are pulled from the ones under the
  FOGLAMP_ROOT directory.
  Please note that you must first run 'make' in the FOGLAMP_ROOT directory.

You may also pass one or more of the following options to cmake to override
this default behaviour:

- **FOGLAMP_SRC** sets the path of a FogLAMP source tree
- **FOGLAMP_INCLUDE** sets the path to FogLAMP header files
- **FOGLAMP_LIB sets** the path to FogLAMP libraries
- **FOGLAMP_INSTALL** sets the installation path of Random plugin

NOTE:
 - The **FOGLAMP_INCLUDE** option should point to a location where all the FogLAMP
   header files have been installed in a single directory.
 - The **FOGLAMP_LIB** option should point to a location where all the FogLAMP
   libraries have been installed in a single directory.
 - 'make install' target is defined only when **FOGLAMP_INSTALL** is set

Examples:

- no options

  $ cmake ..

- no options and FOGLAMP_ROOT set

  $ export FOGLAMP_ROOT=/some_foglamp_setup

  $ cmake ..

- set FOGLAMP_SRC

  $ cmake -DFOGLAMP_SRC=/home/source/develop/FogLAMP  ..

- set FOGLAMP_INCLUDE

  $ cmake -DFOGLAMP_INCLUDE=/dev-package/include ..
- set FOGLAMP_LIB

  $ cmake -DFOGLAMP_LIB=/home/dev/package/lib ..
- set FOGLAMP_INSTALL

  $ cmake -DFOGLAMP_INSTALL=/home/source/develop/FogLAMP ..

  $ cmake -DFOGLAMP_INSTALL=/usr/local/foglamp ..
