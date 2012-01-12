HDF5/iRODS module
------------------------------------------------------------------------
The HDF5/iRODS module is to support interactive access to HDF5 files
stored at iRODS server. This module is disabled by default. To use this
module, you must follow the steps below

	1.  Download and install szip, zlib, and HDF5 libraries from
                http://www.hdfgroup.org/HDF5/release/obtain5.html

		The implementation is compatible with HDF5 1.6.x.
                If you are using HDF5 1.8.x version, you must use
                the 5-1.8.x-16API build. If you are building HDF5 1.8.x
                from the source, you must use the HDF5 1.6 compatibility 
                flag in the configuration, such as
                ./configure --with-default-api-version=v16 ...

	2.  Edit the 'Makefile' in this module and set the follow variables:
                hdf5Dir, szlibDir, and zlibDir.
		Note that hdf5Dir is the install directory of hdf5 created
		by "make install".

                For example,
                hdf5Dir=/home/srb/linux32/hdf5
                szlibDir=/home/srb/ext_lib/szip
                zlibDir=/home/srb/ext_lib/zlib

	3.  Edit the config/config.mk file and add hdf5 to the line that
	    defines MODULES. e.g., 

	        MODULES= properties hdf5

	4.  Recompile iRODS by typing 'make'.  You do not need to
	    run any of the install scripts again.

	5.  Restart iRODS by runing 'irodsctl restart'.


The make also build a test program t5 in the test directory. To run
this test program, first iput a hdf5 data file into irods, e.g.,

	iput hdfTestFile /tempZone/home/rods/hdf5/hdfTestFile

Then type in:

	test/t5 /tempZone/home/rods/hdf5/hdfTestFile


iRODS-HDF5 client API 
---------------------

Five HDF5 microservices: msiH5File_open, msiH5File_close, msiH5Dataset_read, 
msiH5Dataset_read_attribute and msiH5Group_read_attribute have been
implemented on the server. 

The h5ObjRequest() is the primary library call for iRODS clients to access 
the HDF5 objects and micro-services on the iRODS server. It supports 
operations on three types of HDF5 objects:

	H5OBJECT_FILE - hdf5 file
	H5OBJECT_GROUP - hdf5 group
	H5OBJECT_DATASET - hdf5 dataset

hdf5 operations supported are:
	H5FILE_OP_OPEN - open a hdf5 file
	H5FILE_OP_CLOSE - close a hdf5 file
	H5GROUP_OP_READ_ATTRIBUTE - read the attributes of a hdf5 group
	H5DATASET_OP_READ_ATTRIBUTE - read the attributes of a hdf5 dataset

Please read examples given in test/test_h5File.c on how this API is used. 

The client implementation also include a JNI interface given in the
native directory which allows the HFD5 Java browser HDF5View to access 
HDF5 files stored in iRODS.

