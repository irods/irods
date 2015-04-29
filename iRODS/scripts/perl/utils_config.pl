#
# Perl

#
# Standard configuration handling for iRODS Perl scripts.
#
# Usage:
# 	require scripts/perl/utils_config.pl
#
# Dependencies:
# 	scripts/perl/utils_print.pl
# 	scripts/perl/utils_paths.pl
#
# This file provides utility functions to load the iRODS configuration
# files, and derive paths from it.  These functions are
# used by iRODS Perl scripts.
#
# This file sets global variables and prints standard error messages.
#

use File::Spec;
use Cwd "abs_path";

use JSON qw( );
$version{"utils_config.pl"} = "Feb 2015";

# Master defaults for iRODS configuration parameters:
$IRODS_DEFAULT_PORT          = 1247;
$IRODS_DEFAULT_DB_KEY        = 123;
$IRODS_DEFAULT_DB_NAME       = "ICAT";
$IRODS_DEFAULT_RESOURCE_DIR  = "Vault";
$IRODS_DEFAULT_RESOURCE_NAME = "demoResc";
$IRODS_DEFAULT_ZONE          = "tempZone";




$json = JSON->new;
sub load_json_file {
    my ($filename) = @_;
    my $json_text = do {
        open(my $json_fh, "<:encoding(UTF-8)", $filename)
        or die("utils_config.pl:load_json_file: Can't open $filename: $!\n");
        local $/;
        <$json_fh>
    };
    my $data = $json->decode($json_text);
#    print $json->pretty->encode( $data );
    return $data;
}

sub load_server_config {
    my ($filename) = @_;
    # from server_config.json
    $IRODS_ADMIN_NAME     = undef;
    $ZONE_NAME     = undef;
    $IRODS_PORT           = undef;
    $SVR_PORT_RANGE_START = undef;
    $SVR_PORT_RANGE_END   = undef;
    $IRODS_HOST           = undef;
    $RESOURCE_NAME = undef;
    $RESOURCE_DIR  = undef;
    $IRODS_ADMIN_PASSWORD = undef;
    $IRODS_ICAT_HOST = undef;
    $SCHEMA_VALIDATION_BASE_URI = undef;
    # load specific variables
    $data = load_json_file($filename);
    $IRODS_ADMIN_NAME = $data->{'zone_user'};
    $ZONE_NAME = $data->{'zone_name'};
    $IRODS_PORT = $data->{'zone_port'};
    $SVR_PORT_RANGE_START = $data->{'server_port_range_start'};
    $SVR_PORT_RANGE_END = $data->{'server_port_range_end'};
    $IRODS_HOST = getCurrentHostName( );
    $RESOURCE_NAME = $data->{'default_resource_name'};
    $RESOURCE_DIR = $data->{'default_resource_directory'};
    $IRODS_ADMIN_PASSWORD = $data->{'admin_password'};
    $IRODS_ICAT_HOST = $data->{'icat_host'};
    $SCHEMA_VALIDATION_BASE_URI = $data->{'schema_validation_base_uri'};
    # return
    return 1;
}

sub load_database_config {
    my ($filename) = @_;
    # from database_config.json
    $DATABASE_TYPE               = undef;
    $DATABASE_ODBC_TYPE          = undef;
    $DATABASE_HOST               = undef;
    $DATABASE_PORT               = undef;
    $DB_NAME                     = undef;
    $DATABASE_ADMIN_NAME         = undef;
    $DATABASE_ADMIN_PASSWORD     = undef;
    # load specific variables
    $data = load_json_file($filename);
    $DATABASE_TYPE = $data->{'catalog_database_type'};
    $DATABASE_ODBC_TYPE = $data->{'db_odbc_type'};
    $DATABASE_HOST = $data->{'db_host'};
    $DB_NAME = $data->{'db_name'};
    $DATABASE_PORT = $data->{'db_port'};
    $DATABASE_ADMIN_NAME = $data->{'db_user'};
    $DATABASE_ADMIN_PASSWORD = $data->{'db_password'};
    # return
    return 1;
}

sub load_irods_version_file {
    my ($filename) = @_;
    # from VERSION.json
    $CONFIGURATION_SCHEMA_VERSION = undef;
    # load specific variables
    $data = load_json_file($filename);
    $CONFIGURATION_SCHEMA_VERSION = $data->{'configuration_schema_version'};
    # return
    return 1;
}

return( 1 );
