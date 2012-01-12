				iRODS scripts

This directory contains scripts used to support setup and management of
an iRODS installation.

Contents
	configure		configure prior to building iRODS
	finishSetup		finish setting up iRODS
	installPostgres		install Postgres for iRODS
	irodsprompt		prompt for iRODS configuration
	runperl			shell wrapper for perl scripts
	uninstallPostgres	uninstall Postgres for iRODS

Most (all) of these shell scripts are wrappers for Perl scripts in the
'perl' subdirectory.  The Perl scripts are *not* designed to be run
directly.  Please always run the shell script, not the Perl script.
