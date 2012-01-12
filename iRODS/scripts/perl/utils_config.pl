#
# Perl

#
# Standard irods.config handling for iRODS Perl scripts.
#
# Usage:
# 	require scripts/perl/utils_config.pl
#
# Dependencies:
# 	scripts/perl/utils_print.pl
# 	scripts/perl/utils_paths.pl
#
# This file provides utility functions to load the iRODS irods.config
# file, validate it, and derive paths from it.  These functions are
# used by iRODS Perl scripts.
#
# This file sets global variables and prints standard error messages.
#

use File::Spec;
use Cwd "abs_path";

$version{"utils_config.pl"} = "September 2011";





#
# Design Notes:
#	The irods.config file is used by several iRODS Perl scripts
#	as a central place for configuration information they need.
#	This includes the path to the database, the admin account
#	for the database and iRODS, etc.
#
#	Because irods.config is user-editable, we need to do extra
#	error checking to be sure the values make sense, and to fill
#	in defaults if they aren't set at all.
#
#	Scripts that need irods.config should load it with:
#
#		if ( loadIrodsConfig( ) == 0 ) { FAIL }
#
#	This top-level function reads the file and validates it.
#
#	To keep irods.config simple, it is just a Perl script that
#	sets global variables.  This means that loading it here
#	affects the global name space of the iRODS script.  This
#	is not ideal, but since anyone with access to irods.config
#	also has access to the iRODS perl scripts (like this one),
#	security is not really an issue.
#





# Check that dependencies have already been loaded.
if ( ! defined( $irodsConfig ) || ! defined( $IRODS_HOME ) )
{
	print( "\nProgrammer error:\n" );
	print( "    The 'utils_config.pl' Perl script must be loaded after\n" );
	print( "    the other utility scripts.\n" );
	exit( 1 );
}


# Master defaults for iRODS configuration parameters:
$IRODS_DEFAULT_PORT          = 1247;
$IRODS_DEFAULT_DB_KEY        = 123;
$IRODS_DEFAULT_DB_NAME       = "ICAT";
$IRODS_DEFAULT_RESOURCE_DIR  = "Vault";
$IRODS_DEFAULT_RESOURCE_NAME = "demoResc";
$IRODS_DEFAULT_ZONE          = "tempZone";






#
# @brief	Load the iRODS irods.config file.
#
# The irods.config file is used by multiple iRODS Perl scripts as a
# central configuration file for the installed iRODS.  It indicates
# the database in use, the path to that database, and account names
# and passwords for the database and iRODS.
#
# This function finds the file and loads it.  Variables defined in
# the file are added to the global Perl name space.
#
# @return
# 	A numeric code indicating success or failure.
# 		0 = failure
# 		1 = success
#
sub loadIrodsConfig()
{
	return loadIrodsConfigAndValidate(1);
}
sub loadIrodsConfigNoValidate()
{
	return loadIrodsConfigAndValidate(0);
}
sub loadIrodsConfigAndValidate($)
{
	my ($validate) = @_;

	# Check that the configuration file exists.
	if ( ! -e $irodsConfig )
	{
		printError( "\nConfiguration problem:\n" );
		printError( "    The iRODS configuration file is missing.\n" );
		printError( "        Configuration file:  $irodsConfig\n" );
		return 0;
	}


	# Set default values that are overridden by the file.
	$DATABASE_TYPE               = undef;
	$DATABASE_HOME               = undef;
	$DATABASE_LIB                = undef;
	$DATABASE_EXCLUSIVE_TO_IRODS = undef;
	$DATABASE_HOST               = undef;
	$DATABASE_PORT               = undef;
	$DATABASE_ADMIN_NAME         = undef;
	$DATABASE_ADMIN_PASSWORD     = undef;

	$IRODS_ADMIN_NAME     = undef;
	$IRODS_ADMIN_PASSWORD = undef;
	$IRODS_PORT           = undef;
	$SVR_PORT_RANGE_START = undef;
	$SVR_PORT_RANGE_END   = undef;
	$IRODS_HOST           = undef;

	$DB_KEY        = undef;
	$DB_NAME       = undef;
	$ZONE_NAME     = undef;
	$RESOURCE_NAME = undef;
	$RESOURCE_DIR  = undef;

	# Legacy - no longer used
	$ADMIN_NAME  = undef;
	$ADMIN_PW    = undef;
	$DB_PASSWORD = undef;
	$rodsPort    = undef;

	# Load the file.
	require $irodsConfig;

	# Validate it.
	#    While the iRODS configuration file is usually automatically edited
	#    by the installation scripts, it is possible that it has been edited
	#    by a user, or that the scripts have made a mistake.
	if ( $validate == 1 )
	{
		return 0 if validateIrodsVariables( )    == 0;
		return 0 if validateDatabaseVariables( ) == 0;
	}

	# Set environment variables.
	setEnvironmentVariables( );
	return 1;
}





#
# @brief	Validate IRODS variables in irods.config.
#
# This function checks global IRODS_* variables set by irods.config
# to be sure they are properly set.
#
# @return
# 	A numeric code indicating success or failure.
# 		0 = failure
# 		1 = success
#
sub validateIrodsVariables()
{
	# iRODS account:
	#	Check if we have an account name and password..
	#	These are set for most types of installs, but
	#	are allowed to be empty for i-command-only installs.
	my $adminSet = 1;
	if ( !defined( $IRODS_ADMIN_NAME ) || $IRODS_ADMIN_NAME eq "" )
	{
		# No administrator name.  Check legacy names.
		if ( defined( $ADMIN_NAME ) && $ADMIN_NAME ne "" )
		{
			$IRODS_ADMIN_NAME = $ADMIN_NAME;
		}
		else
		{
			$adminSet = 0;
		}
	}
	if ( !defined( $IRODS_ADMIN_PASSWORD ) || $IRODS_ADMIN_PASSWORD eq "" )
	{
		# No adminstrator password.  Check legacy names.
		if ( defined( $ADMIN_PW ) && $ADMIN_PW ne "" )
		{
			$IRODS_ADMIN_PASSWORD = $ADMIN_PW;
		}
		elsif ( defined( $DB_PASSWORD ) && $DB_PASSWORD ne "" )
		{
			$IRODS_ADMIN_PASSWORD = $DB_PASSWORD;
		}
		elsif ( $adminSet == 1 )
		{
			# If an admin account name was given, then
			# we really need the password too.
			printError(
				"\n" .
				"Configuration problem:\n" .
				"    iRODS administrator password is not set.\n" .
				"\n" .
				"    The iRODS configuration file does not set the administrator\n" .
				"    account password.  Possible typo in the variable name?\n" .
				"\n" .
				"    Please check \$IRODS_ADMIN_PASSWORD in the configuration file.\n" .
				"        Config file:  $irodsConfig\n" );
			return 0;
		}
	}


	# iRODS server:
	#	Make sure we have a port.
	#	Make sure we have a host.
	if ( !defined( $IRODS_PORT ) || $IRODS_PORT eq "" )
	{
		# Check legacy name.
		if ( defined( $rodsPort ) && $rodsPort ne "" )
		{
			$IRODS_PORT = $rodsPort;
		}
		else
		{
			# Use the default.
			$IRODS_PORT = $IRODS_DEFAULT_PORT;
		}
	}
	if ( !defined( $IRODS_HOST ) || $IRODS_HOST eq "" )
	{
		$IRODS_HOST = $thisHost;
	}

	# Database password encryption:
	if ( !defined( $DB_KEY ) || $DB_KEY eq "" )
	{
		$DB_KEY = $IRODS_DEFAULT_DB_KEY;
	}

	# Database schema name:
	if ( !defined( $DB_NAME ) || $DB_NAME eq "" )
	{
		$DB_NAME = $IRODS_DEFAULT_DB_NAME;
	}

	# iRODS resource directory:
	if ( !defined( $RESOURCE_DIR ) || $RESOURCE_DIR eq "" )
	{
		$RESOURCE_DIR = File::Spec->catdir( $IRODS_HOME, $IRODS_DEFAULT_RESOURCE_DIR );
	}

	# iRODS resource name:
	if ( !defined( $RESOURCE_NAME ) || $RESOURCE_NAME eq "" )
	{
		$RESOURCE_NAME = $IRODS_DEFAULT_RESOURCE_NAME;
	}

	# iRODS zone:
	if ( !defined( $ZONE_NAME ) || $ZONE_NAME eq "" )
	{
		$ZONE_NAME = $IRODS_DEFAULT_ZONE_NAME;
	}

	return 1;
}





#
# @brief	Validate DATABASE variables in irods.config.
#
# This function checks global DATABASE_* variables set by irods.config
# to be sure they are properly set.
#
# @return
# 	A numeric code indicating success or failure.
# 		0 = failure
# 		1 = success
#
sub validateDatabaseVariables()
{
	# Database type:
	#	Make sure that a database type has been selected.
	#	Make sure it is a supported type.
	if ( $DATABASE_TYPE =~ /postgres/i )
	{
		# Regularize the database type name to make further checks
		# against it easier (i.e., always full name in lower case).
		$DATABASE_TYPE = "postgres";
	}
	elsif ( $DATABASE_TYPE =~ /oracle/i )
	{
		# Regularize the database type name to make further checks
		# against it easier (i.e., always full name in lower case).
		$DATABASE_TYPE = "oracle";
	}
	elsif ( $DATABASE_TYPE =~ /mysql/i )
	{
		# Regularize the database type name to make further checks
		# against it easier (i.e., always full name in lower case).
		$DATABASE_TYPE = "mysql";
	}
	elsif ( $DATABASE_TYPE ne "" )
	{
		printError(
			"\n" .
			"Configuration problem:\n" .
			"    Unsupported database type.\n" .
			"\n" .
			"    The iRODS configuration file indicates a database type\n" .
			"    that is recognized or not supported.  Possible typo?\n" .
			"\n" .
			"    Please check \$DATABASE_TYPE in the configuration file.\n" .
			"        Config file:   $irodsConfig\n" .
			"        Database type: $DATABASE_TYPE\n" );
		return 0;
	}
	else
	{
		# The DATABASE_TYPE is empty.  This is legal and means that
		# the iRODS iCAT server is not being built.  If it isn't
		# being built, compilation doesn't need to know what database
		# to use, and configuration files do not have to adjusted for
		# that database.
	}


	# Database paths:
	#	If there is a $DATABASE_HOME, then there may be a database
	#	under control of the Perl scripts.  Set up the database's
	#	paths and parameters.
	$controlDatabase = 0;
	if ( defined( $DATABASE_HOME ) && $DATABASE_HOME ne "" )
	{
		# Make the database path absolute.
		if ( ! File::Spec->file_name_is_absolute( $DATABASE_HOME ) )
		{
			$DATABASE_HOME = abs_path(
				File::Spec->catfile( $IRODS_HOME, $DATABASE_HOME ) );
		}

		# Even with a database home directory set, if we're sharing
		# a database install then we shouldn't execute database
		# control commands.
		if ( defined( $DATABASE_EXCLUSIVE_TO_IRODS ) &&
			$DATABASE_EXCLUSIVE_TO_IRODS =~ /1|(yes)|(true)/i )
		{
			# Yes, the database is exclusive to iRODS and safe
			# for us to start and stop under Perl script control.
			$controlDatabase = 1;
		}
	}

	$databaseBinDir  = undef;
	$databaseLibDir  = undef;
	$databaseEtcDir  = undef;
	$databaseLogDir  = undef;
	$databaseDataDir = undef;

	$pgctl    = undef;
	$psql     = undef;
	$dropdb   = undef;
	$createdb = undef;
	$vacuumdb = undef;
	$sqlplus  = undef;


	# Make sure the database directory exists if a database type
	# has been specified.
	if ( defined($DATABASE_TYPE) && $DATABASE_TYPE ne "" &&
		(!defined($DATABASE_HOME) || $DATABASE_HOME eq "" || ! -e $DATABASE_HOME) )
	{
		printError(
			"\n" .
			"Configuration problem:\n" .
			"    The database directory is missing!\n" .
			"\n" .
			"    The iRODS configuration indicates a database directory\n" .
			"    but there is no such directory.  Has the database been\n" .
			"    installed yet?  Was it uninstalled?\n" .
			"\n" .
			"    Please check \$DATABASE_HOME in the configuration file.\n" .
			"        Config file:   $irodsConfig\n" .
			"        Database path: $DATABASE_HOME\n" );
		return 0;
	}


	if ( $DATABASE_TYPE eq "postgres" )
	{
		# A common error/confusion is to give a database
		# home directory that is one up from the one to use.
		# For instance, if an iRODS install has put Postgres
		# in "here", then "here/pgsql/bin" is the bin directory,
		# not "here/bin".  Check for this and silently adjust.

		$databaseBinDir  = File::Spec->catdir( $DATABASE_HOME, "bin" );
		if ( ! -e $databaseBinDir )
		{
			my $adjusted    = File::Spec->catdir( $DATABASE_HOME, "pgsql" );
			my $adjustedBin = File::Spec->catdir( $adjusted, "bin" );
			if ( -e $adjustedBin )
			{
				# Yup.  That was it.  Adjust.
				$DATABASE_HOME  = $adjusted;
				$databaseBinDir = $adjustedBin;
			}
			# Otherwise something is wrong with the directory.
			# Fall through to the error message below.
		}

		# Database directories
		$databaseLibDir  = File::Spec->catdir( $DATABASE_HOME, "lib" );
		$databaseEtcDir  = File::Spec->catdir( $DATABASE_HOME, "etc" );
		$databaseLogDir  = $DATABASE_HOME;
		$databaseDataDir = File::Spec->catdir( $DATABASE_HOME, "data" );

		# Database commands
		$pgctl = File::Spec->catfile( $databaseBinDir, "pg_ctl" );
		if ( ! -e $pgctl )
		{
			printError(
				"\n" .
				"Configuration problem:\n" .
				"    Postgres program directory is missing!\n" .
				"\n" .
				"    The iRODS configuration indicates the installed Postgres\n" .
				"    directory, but the files aren't there.  Has the database been\n" .
				"    fully installed?\n" .
				"\n" .
				"    Please check \$DATABASE_HOME in the configuration file.\n" .
				"        Config file:   $irodsConfig\n" .
				"        Database path: $DATABASE_HOME\n" .
				"        Database commands:   $databaseBinDir\n" );
			return 0;
		}
		$psql     = File::Spec->catfile( $databaseBinDir, "psql" );
		$createdb = File::Spec->catfile( $databaseBinDir, "createdb" );
		$createuser = File::Spec->catfile( $databaseBinDir, "createuser" );
		$dropdb   = File::Spec->catfile( $databaseBinDir, "dropdb" );
		$vacuumdb = File::Spec->catfile( $databaseBinDir, "vacuumdb" );

		# Defaults
		if ( !defined( $DATABASE_HOST ) || $DATABASE_HOST eq "" )
		{
			$DATABASE_HOST = "localhost";
		}
		my $thisOS = getCurrentOS( );
		if ( $thisOS =~ /Darwin/i )
		{
			if ( $DATABASE_HOST =~ /\.local$/ )
			{
				# Mac default host names ending in .local
				# are non-standard.  Use "localhost".
				$DATABASE_HOST = "localhost";
			}
		}
		if ( !defined( $DATABASE_PORT ) || $DATABASE_PORT eq "" )
		{
			$DATABASE_PORT = 5432;
		}
	}
	elsif ( $DATABASE_TYPE eq "oracle" )
	{
		# Database directories
		$databaseBinDir  = File::Spec->catdir( $DATABASE_HOME, "bin" );
		if (defined( $DATABASE_LIB ) && $DATABASE_LIB ne "" ) {
		    $databaseLibDir  = File::Spec->catdir( $DATABASE_HOME, $DATABASE_LIB );
		}
		else {
		    $databaseLibDir  = File::Spec->catdir( $DATABASE_HOME, "lib" );
	        }

		# Database commands
		$sqlplus = File::Spec->catfile( $databaseBinDir, "sqlplus" );
		if ( ! -e $sqlplus )
		{
			printError(
				"\n" .
				"Configuration problem:\n" .
				"    Oracle program directory is missing!\n" .
				"\n" .
				"    The iRODS configuration indicates the installed Oracle\n" .
				"    directory, but the files aren't there.  Has the database been\n" .
				"    fully installed?\n" .
				"\n" .
				"    Please check \$DATABASE_HOME in the configuration file.\n" .
				"        Config file:   $irodsConfig\n" .
				"        Database path: $DATABASE_HOME\n" .
				"        Database commands:   $databaseBinDir\n" );
			return 0;
		}
	}
	elsif ( $DATABASE_TYPE eq "mysql" )
	{
		# Use localhost as default
		if ( !defined( $DATABASE_HOST ) || $DATABASE_HOST eq "" )
		{
			$DATABASE_HOST = "localhost";
		}

		# Database directories
		$databaseBinDir  = File::Spec->catdir( $DATABASE_HOME, "bin" );
		if (defined( $DATABASE_LIB ) && $DATABASE_LIB ne "" ) {
		    $databaseLibDir  = File::Spec->catdir( $DATABASE_HOME, $DATABASE_LIB );
		}
		else {
		    $databaseLibDir  = File::Spec->catdir( $DATABASE_HOME, "lib" );
		}

		# Database commands
		$odbcinst = File::Spec->catfile( $databaseBinDir, "odbcinst" );
		if ( ! -e $odbcinst )
		{
			printError(
				"\n" .
				"Configuration problem:\n" .
				"    ODBC program directory is missing!\n" .
				"\n" .
				"    The iRODS configuration indicates the installed ODBC\n" .
				"    directory, but the files aren't there.  Has the database been\n" .
				"    fully installed?\n" .
				"\n" .
				"    Please check \$DATABASE_HOME in the configuration file.\n" .
				"        Config file:   $irodsConfig\n" .
				"        Database path: $DATABASE_HOME\n" .
				"        Database commands:   $databaseBinDir\n" );
			return 0;
		}
        
        # buld parameter list for mysql - run odbcinst with the specified 
        # data source name and extrac connection parameters for mysql
        if ( ! open ( ODBCINST, "$odbcinst -q -s -n \"$DB_NAME\" 2>&1 |" ) ) {
			printError(
				"\n" .
				"Configuration problem:\n" .
				"    failed to run odbcinst application!\n" .
				"\n" .
				"    Please check \$DATABASE_HOME in the configuration file.\n" .
				"        Config file:   $irodsConfig\n" .
				"        Database path: $DATABASE_HOME\n" .
				"        Database commands:   $databaseBinDir\n" );
			return 0;
        }
		$mysql = "" ;
		my $option = 0;
		my $line;
		foreach $line ( <ODBCINST> ) {
			chomp( $line );
			#
			# workaround for odbcinst bug - it can "join" the lines if there
			# is no value for the parameter
			#
			if ( $line =~ /=/ ) {
				my @words = split ( /\s*=\s*/, $line, -1 );
				
				# If the last word is non-empty then it is the value and 
				# preceeding one is a key
				my $val = $words[$words-1] ;
				if ( $val ne "" ) {
					my $key = lc( $words[$words-2] );
					if ( $key eq "server" ) {
						$mysql = " -h\"" . $val . "\"" . $mysql;
					} elsif ( $key eq "port" ) {
						$mysql = " -P" . $val . $mysql;
					} elsif ( $key eq "database" ) {
						$mysql = $mysql . " \"" . $val . "\"" ;
					} elsif ( $key eq "option" ) {
						$option = $val;
					}
				}
			}
		}

		close ( ODBCINST ) ;

		# Verify that we have correct option passed to the driver, we need CLIENT_FOUND_ROWS=0x2
		# which make it return "found rows" instead of "affected rows" for UPDATE statements.
		if ( ! ( $option & 0x2 ) ) {
			printError(
				"\n" .
				"Configuration problem:\n" .
				"    CLIENT_FOUND_ROWS is not specified in MySQL ODBC driver options!\n" .
				"\n" .
				"    Please check the value of the \"Option\" in the odbc.ini and\n" .
				"    set its value to contain mask 0x2, e.g. \"Option = 2\".\n" );
			return 0;
		}
		
		# 
		# Good version of mysql command must be found in a path
		#
		$mysql = "mysql  -u\"$DATABASE_ADMIN_NAME\" -p\"$DATABASE_ADMIN_PASSWORD\" --skip-column-names " . $mysql;
	}
	else
	{
		# Unrecognized database.  If we don't know what
		# it is, we can't control it.
		$controlDatabase = 0;
	}

	return 1;
}






#
# @brief	Set environment variables for running commands.
#
# Many of the database commands used by these Perl scripts can or
# must get parameters from the environment.  These include the
# name of the database, the path to the database directory, the
# database host, the database port, etc.
#
# This function sets environment variables.  The exact variables
# set depends upon the DATABASE_TYPE selected and other values
# set by the iRODS configuration file.
#
# @return
# 	A numeric code indicating success or failure.
# 		0 = failure
# 		1 = success
#
sub setEnvironmentVariables
{
	# Execution path:
	#	Add "." to help running scripts.
	#	Add the iRODS commands.
	#	Add the database commands.
	my $addPath = ".:$icommandsBinDir";
	if ( defined( $databaseBinDir ) && $databaseBinDir ne "" )
	{
		$addPath .= ":$databaseBinDir";
	}
	$ENV{'PATH'} = "$addPath:" . $ENV{'PATH'};


	# Library path:
	#	Add the database libraries.
	#	Add /usr/local/lib, needed on Solaris.
	my $libPath = $ENV{'LD_LIBRARY_PATH'};  
	if ( defined( $databaseLibDir ) && $databaseLibDir ne "" )
	{
		if ( $libPath eq "" )
		{
			$libPath = "$databaseLibDir";
		}
		else
		{
			$libPath = "$databaseLibDir:$libPath";
		}
		
	}
	my $locallib = File::Spec->catdir( File::Spec->rootdir( ), "usr", "local", "lib" );
	if ( $libPath eq "" )
	{
		$libPath = $locallib;
	}
	else
	{
		$libPath = "$libPath:$locallib";
	}
	$ENV{'LD_LIBRARY_PATH'} = $libPath;


# If the GLOBUS_LOCATION (in irods.config) is set, add it/lib to the
# LD_LIBRARY_PATH if it is not there already.  This can be useful when
# GSI is configured in but is not being used initially, so the user's
# GSI environment might not be set up.
	if ( defined($GLOBUS_LOCATION) && $GLOBUS_LOCATION ne "" )
	{
	    if ( $libPath =~ m/$GLOBUS_LOCATION/i ) {
	    }
	    else {
		if ( $libPath eq "" )
		{
			$libPath = $GLOBUS_LOCATION . "/lib" ;
		}
		else
		{
			$libPath = $GLOBUS_LOCATION . "/lib:" . $libPath;
		}
	    }
	    $ENV{'LD_LIBRARY_PATH'} = $libPath;
	}


	# Database variables
	if ( $DATABASE_TYPE eq "postgres" )
	{
		# Postgres database

		# The path to the data directory
		$ENV{"PGDATA"} = $databaseDataDir;

		# The server's port.
		$ENV{"PGPORT"} = $DATABASE_PORT;

		# The server's host.  If the host is the current
		# host, then don't set this.  This forces commands
		# to use a local socket instead of a TCP/IP connection.
		if ( $DATABASE_HOST !~ "localhost" &&
			$DATABASE_HOST !~ $thisHost )
		{
			$ENV{"PGHOST"} = $DATABASE_HOST;
		}

		# The user's name.
		$ENV{"PGUSER"} = $DATABASE_ADMIN_NAME;

		# The user's password.
		$ENV{"PGPASSWORD"} = $DATABASE_ADMIN_PASSWORD;
	}
	if ( $DATABASE_TYPE eq "oracle" )
	{
	    $ENV{"ORACLE_HOME"} = $DATABASE_HOME;
	}
}





return( 1 );
