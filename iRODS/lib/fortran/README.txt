DIRECTORY
	iRODS/lib/fortran    - Interface library for FORTRAN applications

DESCRIPTION 
        This directory contains an interface library (and test/example
	programs) which can be used by Fortran applications to do
	direct (thru the network) I/O via the iRODS C client library.
	See the comment blocks in the test programs and library source
	file (src/fortran_io.c) for more.  

        This is a simple and basic interface library, implemented as a
	correlation to iRODS C code.  If needed, the API could be
	defined differently.  But this simple interface may be used
	for testing and analysis.
