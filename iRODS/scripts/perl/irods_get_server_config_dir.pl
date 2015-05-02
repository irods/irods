use Cwd "abs_path";
use File::Basename;
use File::Spec;

$scriptfullpath = abs_path(__FILE__);
$scripttoplevel = dirname(dirname(dirname(dirname($scriptfullpath))));

if ( -e "$scripttoplevel/packaging/binary_installation.flag" )
{
        $configDir = "/etc/irods";
}
else
{
        $configDir = File::Spec->catdir( "$scripttoplevel", "iRODS", "server", "config" );
}

print "$configDir";
