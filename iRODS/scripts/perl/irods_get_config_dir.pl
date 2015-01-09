use Cwd "abs_path";
use File::Basename;
use File::Spec;

$scriptfullpath = abs_path(__FILE__);
$scripttoplevel = dirname(dirname(dirname(dirname($scriptfullpath))));

if( -e "/etc/irods/server_config.json" )
{
        $configDir = "/etc/irods";
}
else
{
        $configDir = File::Spec->catdir( "$scripttoplevel", "iRODS", "config" );
}

print "$configDir";
