				iRODS scripts

This directory contains scripts used to support setup and management of
an iRODS installation.

Contents
	configure		configure prior to building iRODS
	irodsprompt		prompt for iRODS configuration
	runperl			shell wrapper for perl scripts

Most (all) of these shell scripts are wrappers for Perl scripts in the
'perl' subdirectory.  The Perl scripts are *not* designed to be run
directly.  Please always run the shell script, not the Perl script.
