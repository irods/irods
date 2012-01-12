				iRODS Perl scripts

This directory contains Perl scripts used to support setup and management of
an iRODS installation.

Contents
	configure.pl		configure prior to building iRODS
	finishSetup.pl		finish setting up iRODS
	installPostgres.pl	install Postgres for iRODS
	irodsctl.pl		start or stop the iRODS servers
	irodsprompt.pl		prompt for config settings
	irodssetup.pl		set up iRODS
	uninstallPostgres.pl	uninstall Postgres for iRODS

	utils_config.pl		utility functions for irods.config
	utils_file.pl		utility functions for file I/O
	utils_paths.pl		utility functions for standard paths
	utils_platform.pl	utility functions for OS dependencies
	utils_print.pl		utility functions for message printing
	utils_prompt.pl		utility functions for prompting

Most (all) of these perl scripts are intended to be run by wrapper shell
scripts in the directory above.
