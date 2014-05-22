#
# Perl

#
# Standard directory and file paths for iRODS Perl scripts.
#
# Usage:
# 	require scripts/perl/utils_paths.pl
#
# Dependencies:
# 	none
#
# This file initializes variables for common file paths used by
# the iRODS Perl scripts.
#
# This script sets global variables.
#

use File::Spec;
use File::Basename;

$version{"utils_paths.pl"} = "September 2011";



#
# Detect run-in-place installation
#
$RUNINPLACE = 0;
if ( ! -e "/etc/irods/irods.config" )
{
        $RUNINPLACE = 1;
}


#
# iRODS installation directories
#
$perlScriptsDir   = File::Spec->catdir( $IRODS_HOME, "scripts", "perl" );
$modulesDir       = File::Spec->catdir( $IRODS_HOME, "modules" );

$icommandsBinDir  = File::Spec->catdir( $IRODS_HOME, "clients", "icommands", "bin" );
$icommandsTestDir = File::Spec->catdir( $IRODS_HOME, "clients", "icommands", "test" );

$serverBinDir     = File::Spec->catdir( $IRODS_HOME, "server",  "bin" );
$serverSqlDir     = File::Spec->catdir( $IRODS_HOME, "server",  "icat", "src" );
$serverAuditExtSql= File::Spec->catdir( $IRODS_HOME, "server",  "icat", "auditExtensions" );
$extendedIcatDir  = File::Spec->catdir( $IRODS_HOME, "modules",  "extendedICAT" );
$serverTestBinDir = File::Spec->catdir( $IRODS_HOME, "server",  "test", "bin" );
$serverTestCLLBinDir = $serverTestBinDir;
$serverConfigDir  = File::Spec->catdir( $IRODS_HOME, "server",  "config" );

$logDir           = File::Spec->catdir( $IRODS_HOME, "installLogs" );





#
# iRODS user directories
#
$userIrodsDir     = File::Spec->catfile( $ENV{"HOME"}, ".irods" );
$userIrodsFile    = File::Spec->catfile( $userIrodsDir, ".irodsEnv" );





#
# iRODS commands
#
$iinit  = File::Spec->catfile( $icommandsBinDir, "iinit" );
$iexit  = File::Spec->catfile( $icommandsBinDir, "iexit" );
$iadmin = File::Spec->catfile( $icommandsBinDir, "iadmin" );
$ils    = File::Spec->catfile( $icommandsBinDir, "ils" );
$iput   = File::Spec->catfile( $icommandsBinDir, "iput" );
$iget   = File::Spec->catfile( $icommandsBinDir, "iget" );
$irm    = File::Spec->catfile( $icommandsBinDir, "irm" );
$ichmod = File::Spec->catfile( $icommandsBinDir, "ichmod" );

$irodsctl = File::Spec->catfile( $IRODS_HOME, "irodsctl" );





#
# iRODS configuration files
#
$configMk    = File::Spec->catfile( $configDir, "config.mk" );

# =-=-=-=-=-=-=-
# search for the irods.config file
$irodsConfig = File::Spec->catfile( $configDir, "irods.config" );


#
# Overrides for run-in-place installations
#
if ( $RUNINPLACE == 1 )
{
        $serverSqlDir     = File::Spec->catdir( dirname($IRODS_HOME), "plugins",  "database", "src" );
        $serverTestCLLBinDir = File::Spec->catdir( dirname($IRODS_HOME), "plugins", "database" );
}









return( 1 );
