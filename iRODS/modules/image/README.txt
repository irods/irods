Image Microservices
------------------------------------------------------------------------

This is an early draft implementation of image manipulation microservices
based upon ImageMagick.  The service's names and arguments may change in
the future.

This module is disabled by default.

To try this module:
	1.  Install ImageMagick.  Source can be downloaded from:
		http://www.imagemagick.org/

	2.  Edit the 'Makefile' in this module and set the IMAGEMAGICK
	    variable to the path to the ImageMagick home directory.

	3.  Enable this module by editing 'info.txt' in this directory
	    and change:
		Enabled: no
	    to
		Enabled: yes

	4.  Recompile iRODS by typing 'make'.  You do not need to
	    run any of the install scripts again.

	5.  Restart iRODS by runing 'bin/irodsctl restart'.
