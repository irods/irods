ERA
-----------------------------------------------------------------------------------------------------------

The ERA module is an early implementation of NARA's requirements for building an Electronic Records Archive.

As of now it consists of a set of microservices and utilities for creating, expanding or replicating 
a preservation environment.

They provide a way to import and export:
	* data
	* metadata
	* access control
	* user information
either from raw data or from data from a previous SRB or iRODS based preservation environment.

This is a work in progress and as such is subject to extensive modifications. In particular this module 
may later be broken down into separate modules (eg: metadata, preservation, audit, etc...).

Current dependencies: None

Default status: Disabled

The ERA module does not require additional software. It can be enabled by editing its info.txt file or
by running: IRODS_DIR/scripts/configure --enable-ERA
