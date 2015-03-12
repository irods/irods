#
# Perl

#
# Platform functions used by iRODS perl scripts.
#
# Usage:
#       require scripts/perl/utils_platform.pl
#
# Dependencies:
#       scripts/perl/utils_paths.pl
#       scripts/perl/utils_print.pl
#
# The functions in this file either (a) detect host platform
# features, or (b) invoke those features.  In either case, they
# cover up some platform-dependencies and enable other scripts
# to be more generic.
#
# Platform dependencies values include getting the name of the OS,
# the user, the processor type, and particularly the host name
# and IP addresses.
#

use Sys::Hostname;
use IO::Socket;
use Cwd;
use File::Spec;
use Socket;
use POSIX;
use Config;

eval { require Net::hostent; };
if ( $@ )
{
        print( "Perl installation problem:\n" );
        print( "    The Perl Net::hostent module is not installed.  This module\n" );
        print( "    is required by the iRODS installation scripts.  Please\n" );
        print( "    contact your system administrator.\n" );
        exit( 1 );
}
Net::hostent->import( );

eval { require Net::FTP; };
if ( $@ )
{
        print( "Perl installation problem:\n" );
        print( "    The Perl Net::FTP module is not installed.  This module\n" );
        print( "    is required by the iRODS installation scripts.  Please\n" );
        print( "    contact your system administrator.\n" );
        exit( 1 );
}
Net::FTP->import( );

$version{"utils_platform.pl"} = "September 2011";





#
# Design Notes:  getting host attributes
#       Determining OS and host-specific features has to be done
#       without actually using OS-specific features.  For example,
#       we can't use a UNIX command to determine if we are on UNIX!
#
#       Instead, we try to use built-in Perl features wherever
#       possible, and only fall back to external commands when
#       nothing else is available.  However, while Perl has many
#       modules for this sort of thing, we can only use features
#       built into Perl or in the standard Perl distribution.
#       Optional modules may not be installed on this host, so
#       we can't use them.
#
#       The POSIX module *is* standard and provides the uname()
#       function, which is similar to the UNIX uname command.  The
#       function returns:
#               $sysname  = OS name.
#               $nodename = host name.
#               $release  = OS release number.
#               $version  = OS version number.
#               $machine  = processor class.
#
#       The same values are returned by the uname command:
#               uname -s  = OS name.
#               uname -r  = OS release number.
#               uname -v  = OS vedrsion number.
#               uname -n  = host name.
#               uname -m  = processor class.
#               uname -p  = processor type.
#
#       Notices that the command's -p (processor type) has no
#       equivalent in the uname() function.  Sample results:
#
#       Linux (RedHat):
#               sysname         Linux
#               nodename        qe2
#               release         2.4.21-51.Elsmp
#               version         #1 SMP Wed Jul 25 03:27:17 EDT 2007
#               machine         i686
#               (uname -p       i686)
#
#       Linux (Ubuntu):
#               sysname         Linux
#               nodename        whatever
#               release         2.6.15-29-386
#               version         #1 PREEMPT Mon Sep 24 17:18:25 UTC 2007
#               machine         i686
#               (uname -p       unknown)
#
#       Mac OS X (Intel):
#               sysname         Darwin
#               nodename        Cantaga.local
#               release         9.1.0
#               version         Darwin Kernel Version 9.1.0: Wed Oct 31 17:46:22 PDT 2007; root:xnu-1228.0.2~1/RELEASE_I386
#               machine         i386
#               (uname -p       i386)
#
#       Mac OS X (PPC):
#               sysname         Darwin
#               nodename        fileserver
#               release         8.10.0
#               version         Darwin Kernel Version 8.10.0: Wed May 23 16:50:59 PDT 2007; root:xnu-792.21.3~1/RELEASE_PPC
#               machine         Power Macintosh
#               (uname -p       powerpc)
#
#       Solaris:
#               sysname         SunOS
#               nodename        multivac
#               release         5.9
#               version         Generic_118558-37
#               machine         sun4u
#               (uname -p       sparc)
#
#       Note that $sysname is reliable for the OS name
#       ("Darwin" = Mac OS X).  $nodename is no better than other
#       methods to get the host name.  $release and $version are
#       too illdefined to be very useful.
#
#       $machine works to get a rough processor class, but don't
#       depend upon it with much precision.  For example, on a Mac
#       running Mac OS X, $machine is "i386", but on the *same* Mac
#       with Ubuntu Linux via Parallels virtualization, $machine
#       is now "i686".
#
#       Also note that 'uname -p' is *not* a reliable way to get
#       the processor type.  On Ubuntu Linux, it returned "unknown",
#       when $machine was "i686".
#
#
# Design Notes:  caching
#       Separately, some of these functions are expensive.  They may
#       run external commands, which require starting up a shell.  To
#       save time, many of these functions cache the results from a
#       first call and simply return it on subsequent calls.
#
#
# Design Notes:  finding external commands
#       The user's $PATH may not include the commands we want.  To
#       be safer, the findCommand() function here looks in $PATH,
#       and then onwards through standard UNIX directories to find
#       a desired command.  Other functions that run such commands
#       use findCommand() to find the path to it.
#





# Make sure the paths utility script has been loaded.
if ( defined( $IRODS_HOME ) )
{
        require File::Spec->catfile( $IRODS_HOME, "scripts", "perl", "utils_print.pl" );
}





#
# @brief        Return the path to a command.
#
# This function looks for the given command to see if it can be found
# on this host.  Linux and Mac OS X distributions, for instance,
# sometimes come without a C compiler, make, etc.  Instead of waiting
# for something major to fail, use this function to check ahead of
# time that a needed command is available.
#
# @param        $command
#       the command to look for
# @return
#       the path to the command, or undef if not found
#
sub findCommand($)
{
        my ($command) = @_;

        # Principal directories in a UNIX distribution in which
        # to find commands.
        my @baseDirectories = (
                # Standard UNIX directories
                "bin",
                "sbin",
                "etc",
                File::Spec->catdir( "usr", "bin" ),
                File::Spec->catdir( "usr", "sbin" ),
                # Additional Solaris directories
                File::Spec->catdir( "usr", "ucb" ),
                File::Spec->catdir( "usr", "ccs", "bin" ),
                # Additional Mac directories
                File::Spec->catdir( "Developer", "usr", "bin" ),
                File::Spec->catdir( "Developer", "usr", "sbin" ),
                # Additional local UNIX directories
                File::Spec->catdir( "usr", "local", "bin" ),
                File::Spec->catdir( "usr", "local", "apps", "bin" ),
                File::Spec->catdir( "usr", "share", "bin" ),
        );

        # Principal root directories.  For UNIX, simply / is
        # enough.  But also look in Fink (/sw) and MacPorts (/opt)
        # directories, plus Cygwin (/cygwin) on Windows.
        my @rootDirectories = (
                File::Spec->rootdir( ),
                File::Spec->catdir( File::Spec->rootdir( ), "sw" ),
                File::Spec->catdir( File::Spec->rootdir( ), "opt" ),
                File::Spec->catdir( File::Spec->rootdir( ), "cygwin" ),
        );

        # First, check the user's PATH via 'which'
        $whichPath = `which $command 2> /dev/null`;
        chomp($whichPath);
        if (-f $whichPath && -x $whichPath ) {  # a file and executable
                return $whichPath;
        }

        # Loop through the possible roots.
        foreach $root (@rootDirectories)
        {
                # Loop through the possible paths from the root.
                foreach $base (@baseDirectories)
                {
                        # Does the directory exist?
                        my $dir = File::Spec->catdir( $root, $base );
                        next if ( ! -e $dir );

                        # Does the command exist in that directory?
                        $path = File::Spec->catfile( $dir, $command );
                        next if ( ! -f $path && ! -l $path );   # not file or symlink
                        next if ( ! -x $path );         # not executable

                        # Found!
                        return $path;
                }
        }

        # Not found!
        return undef;
}





########################################################################
#
# Platform attributes
#

#
# @brief        Get the name of the current user.
#
# Find the login name of the current user and return it.  While it is
# unlikely that this function will fail, if it does it returns undef.
#
# @return
#       the current user name, or undef if not known.
#
my $USER = undef;
sub getCurrentUser()
{
        # Return the cached value.
        return $USER if defined( $USER );

        # Check the environment first.
        # Both $USER and $LOGNAME are commonly used
        # (and often both).
        $USER = $ENV{"USER"};
        return if ( defined( $USER ) && $USER != "" );

        $USER = $ENV{"LOGNAME"};
        return if ( defined( $USER ) && $USER != "" );

        # Otherwise, try a UNIX command.
        my $whoami = findCommand( "whoami" );
        if ( defined( $whoami ) )
        {
                $USER = `$whoami`;
                chomp( $USER );
        }

        # If all that fails, $USER will be undef.
        return $USER;
}





#
# @brief        Get host-specific attributes.
#
# Get and cache the OS name, processor type, and host name.
#
my $OS = undef;
my $PROCESSOR = undef;
my $HOSTNAME = undef;
sub _getCurrentHostAttributes()
{
        # Get the OS and processor names.
        #
        # Check the POSIX uname function first.  See design
        # notes at the top of this file for these values.
        my ($sysname, $nodename, $release, $version, $machine) = POSIX::uname( );
        if ( $sysname ne "" )
        {
                $OS = $sysname;
        }
        if ( $machine ne "" )
        {
                $PROCESSOR = $machine;
        }

        # Otherwise, try a UNIX command.
        if ( !defined( $OS ) || !defined( $PROCESSOR ) )
        {
                my $uname = findCommand( "uname" );
                if ( !defined( $OS ) )
                {
                        $OS = `$uname -s`;
                        if ($? ne 0) {
                            print("The simple running of a sub-process has failed, indicating that\n");
                            print("something is fundamentally wrong in the Perl environment.  Since this\n");
                            print("will cause a variety of other problems later, the installation is\n");
                            print("being aborted now.  This has been seen on a CentOS 5.2 host and\n");
                            print("when the FTP module was enabled (the 'require Net::FTP;'\n");
                            print("above), the sub-process calls would start to fail (via either\n");
                            print("back ticks as above, or 'system' function calls.\n");
                            exit ( 1 );
                        }
                        chomp( $OS );
                }
                if ( !defined( $PROCESSOR ) )
                {
                        # On many hosts "uname -m" returns the same value,
                        # but not on Solaris.  Sample values:
                        #                       -m      -p
                        #       Mac OS X        i386    i386
                        #       Linux           i686    i686
                        #       Solaris PC      i86pc   i386
                        #       Solaris SPARC   sun4u   sparc
                        $PROCESSOR = `$uname -p`;
                        chomp( $PROCESSOR );
                }
        }


        # Get the host name.
        #
        # hostname() from Sys::Hostname tries these:
        #       C's gethostname()
        #       `$Config{aphostname}`
        #       uname()
        #       syscall(SYS_gethostname)
        #       `hostname`
        #       `uname -n`
        #       /com/host
        #
        # In practice, it usually returns the simple host
        # name, without domain or .com/.edu qualifiers.
        #
        # So, a typical result might be "myhost".  On a Mac,
        # it may return "myhost.local".
        my $simpleName = hostname( );

        # We want the fully-qualified host name, if possible.
        # There might not be such a thing!  A laptop or home
        # user may not have DNS entries for their hosts, so
        # there won't be a fully-qualified name.  But we
        # try anyway.
        #
        # getHost() from Net::hostent is supposed to return
        # a host record found from a DNS server or in
        # /etc/hosts.  However, for a computer without a DNS
        # entry and no /etc/hosts (very common), this will
        # return nothing.
        #
        # If it works, though, we'll get something like
        # "myhost.biguniv.edu".
        my $hostEntry  = gethost( $simpleName );
        if ( ! defined( $hostEntry ) )
        {
                # Nothing.  This computer probably does not
                # have a DNS or /etc/hosts entry.
                $HOSTNAME = $simpleName;
        }
        else
        {
                # Use the entry's fully-qualified name.
                $HOSTNAME = $hostEntry->name;
        }

        if ( getCurrentOS( ) =~ /Darwin/i )     # Mac OS X
        {
                # Macs have a default host name of the form:
                #       NAME.local
                # when they don't have a DNS entry.  This
                # includes home computers, laptops, or work
                # computers in offices.  The above methods
                # will get this name instead of something usable.
                #
                # Try one more way.  Get the IP addresses of
                # all of the network interfaces and look up
                # their names by reverse DNS.  This may still
                # fail.
                if ( $HOSTNAME =~ /\.local$/i )
                {
                        # Get IP addresses.
                        my %addresses = getHostAddresses( $HOSTNAME );

                        # Pick the first one that doesn't have
                        # a .local name, if any.
                        foreach $address (keys %addresses)
                        {
                                if ( $HOSTNAME !~ /\.local$/i )
                                {
                                        $HOSTNAME = $addresses{$address};
                                        last;
                                }
                        }

                        # If that didn't find anything, then a
                        # .local name is a non-standard host name.
                        # Since there is apparently no DNS entry
                        # for this host, just use 'localhost'.
                        if ( $HOSTNAME =~ /\.local$/i )
                        {
                                $HOSTNAME = "localhost";
                        }
                }
                # Even if this Mac is currently connected to a
                # network, if it is a laptop it very well might
                # have a different name with the next wireless
                # connection.  In that case, we're better off
                # just using localhost so that as it changes,
                # postgres and iRODS will still work.
                # If the user sets one of these environment variables,
                # we'll use localhost as the host name in place
                # of the full DNS name.
                # Ubuntu works differently so this is just for Macs.
                $USE_LOCAL = $ENV{"USE_LOCAL"};
                if ( defined( $USE_LOCAL ) && $USE_LOCAL != "" )
                {
                    $HOSTNAME = "localhost";
                }
                $USE_LOCALHOST = $ENV{"USE_LOCALHOST"};
                if ( defined( $USE_LOCALHOST ) && $USE_LOCALHOST != "" )
                {
                    $HOSTNAME = "localhost";
                }
                $USE_LOCAL_HOST = $ENV{"USE_LOCAL_HOST"};
                if ( defined( $USE_LOCAL_HOST ) && $USE_LOCAL_HOST != "" )
                {
                    $HOSTNAME = "localhost";
                }
        }

# Also allow this override on non-Mac hosts too.
# If USE_LOCALHOST is defined, and non-empty, use "localhost" as the
# host name.
#
# The $USE_LOCAL_HOST != "" doesn't work correctly on some hosts
# (Ubuntu at least) altho it does on Macs (at least some), so use the
# alternative !$USE_LOCALHOST == "" form.
        $USE_LOCALHOST = $ENV{"USE_LOCALHOST"};
        if ( defined( $USE_LOCALHOST ) && !$USE_LOCALHOST == "" )
        {
            $HOSTNAME = "localhost";
        }

#
# Another override; let the user set their local host name, for
# example, to an IP address.  This can be used in cases where the DNS
# does not have an entry (useful for CineGrid).
#
        $LOCAL_HOST_NAME = $ENV{"LOCAL_HOST_NAME"};
        if ( defined( $LOCAL_HOST_NAME ) && !$LOCAL_HOST_NAME == "" )
        {
            $HOSTNAME = $LOCAL_HOST_NAME;
        }
}





#
# @brief        Get the name of the current OS.
#
# @return
#       the current OS name
#
sub getCurrentOS()
{
        # Return the cached value.
        return $OS if defined( $OS );

        _getCurrentHostAttributes( );
        return $OS;
}





#
# @brief        Get the name of the current processor type.
#
# @return
#       the current processor type (e.g. "i386").
#
sub getCurrentProcessor()
{
        # Return the cached value.
        return $PROCESSOR if defined( $PROCESSOR );

        _getCurrentHostAttributes( );
        return $PROCESSOR;
}





#
# @brief        Get the name of the current host.
#
# @return
#       the current host name
#
sub getCurrentHostName()
{
        # Return the cached value.
        return $HOSTNAME if defined( $HOSTNAME );

        _getCurrentHostAttributes( );
        return $HOSTNAME;
}





#
# @brief        Get the IP addresses and names used by
#               the current host.
#
# A single host may have multiple NICs and multiple IP
# addresses.  This call tries to get a complete list of
# those addresses and returns them, along with the names
# bound to each address.
#
# @return
#       an associative array where the keys are IP addresses
#       and the values are host names.
#
sub getCurrentHostAddresses()
{
        my $hostname = getCurrentHostName( );
        return getHostAddresses( getCurrentHostName() );
}

sub getHostAddresses($)
{
        my ($hostname) = @_;

        # Get a list of IP addresses for this host.
        # A single host may have multiple network
        # interfaces, and thus multiple IP addresses.

        # Use 'ifconfig' if available (it nearly always is).
        # This returns a list of all NICs and their IP
        # addresses.  Get each IP address, then look up
        # the name associated with it.
        #
        # The format of ifconfig output varies from OS to OS.
        #
        # Linux (RedHat):
        #       eth0    Link encap:Ethernet  HWaddr 00:11:43:D6:30:E3
        #               inet addr:132.249.20.50  Bcast:132.249.20.255  Mask:255.255.255.0
        #               UP BROADCAST RUNNING SLAVE MULTICAST  MTU:1500  Metric:1
        #               ...
        #
        # Linux (Ubuntu):
        #       eth0    Link encap:Ethernet  HWaddr 00:1C:42:81:45:97
        #               inet addr:10.211.55.8  Bcast:10.211.55.255  Mask:255.255.255.0
        #               UP BROADCAST RUNNING SLAVE MULTICAST  MTU:1500  Metric:1
        #               ...
        #
        # Mac:
        #       en0: flags=8863<UP,BROADCAST,SMART,RUNNING,SIMPLEX,MULTICAST> mtu 1500
        #               inet6 fe80::216:cbff:fe9f:7f57%en0 prefixlen 64 scopeid 0x4
        #               inet 192.168.1.101 netmask 0xffffff00 broadcast 192.168.1.255
        #               ...
        # Solaris:
        #       bge0: flags=9040843<UP,BROADCAST,RUNNING,MULTICAST,DEPRECATED,IPv4,NOFAILOVER> mtu 1500 index 2
        #               inet 132.249.20.84 netmask fffffe00 broadcast 132.249.21.255
        #               groupname multivac
        #               ...
        #
        #  We don't care about the name of the interface or any of
        #  its flags.  All we want are IP4 network addresses.
        #  Fortunately, for all of these OSes, these lines always
        #  start with 'inet '.
        #
        my $ifconfig = findCommand( "ifconfig" );
        if ( defined( $ifconfig ) )
        {
                my $output = `$ifconfig -a 2>&1`;
                if ( ($?>>8) == 0 )
                {
                        # Get IP addresses.
                        my @lines = split( "\n", $output );
                        my @iplist = ();
                        foreach $line ( @lines )
                        {
                                if ( $line =~ /inet (addr:)?([0-9\.]+) /i )
                                {
                                        push( @iplist, $2 );
                                }
                        }

                        # Get names for IP addresses.
                        my $hostlist;
                        foreach $ip ( @iplist )
                        {
                                my $hostEntry = gethost( $ip );
                                if ( defined( $hostEntry ) )
                                {
                                        $hostlist{$ip} = $hostEntry->name;
                                }
                                else
                                {
                                        $hostlist{$ip} = $hostname;
                                }
                        }
                        return %hostlist;
                }
        }

        # Use 'gethost'.  Unfortunately, while this has
        # the ability to return multiple IP addresses,
        # it usually does not.
        my @iplist = ();
        my $hostEntry = gethost( $hostname );
        if ( defined( $hostEntry ) )
        {
                foreach $rawIP (@{$hostEntry->addr_list})
                {
                        push( @iplist, inet_ntoa( $rawIP ) );
                }
        }

        # Get names for IP addresses.
        my $hostlist;
        foreach $ip ( @iplist )
        {
                my $hostEntry = gethost( $ip );
                if ( defined( $hostEntry ) )
                {
                        $hostlist{$ip} = $hostEntry->name;
                }
                else
                {
                        $hostlist{$ip} = $hostname;
                }
        }
        return %hostlist;
}

########################################################################
#
# Platform programs
#

#
# @brief        Get the make command for this OS.
#
# @return
#       the current make command
#
my $MAKE = undef;
sub getCurrentMake()
{
        # Return the cached value.
        return $MAKE if defined( $MAKE );

        # We want 'gmake'.
        $MAKE = findCommand( "gmake" );

        # If that can't be found, try for 'make'.  On a Mac,
        # 'make' is actually 'gmake' and there is no 'gmake'.
        if ( !defined($MAKE) )
        {
                $MAKE = findCommand( "make" );
        }

        # This may not work if neither 'make' or 'gmake' are
        # installed.  This is the case for Linux or Mac OS X
        # without the developer tools installed.  In that case,
        # we return undef.
        return $MAKE;
}





#
# @brief        Get the C compiler command for this OS.
#
# @return
#       the current C compiler command
#
my $CC = undef;
sub getCurrentCC()
{
        # Return the cached value.
        return $CC if defined( $CC );

        # We want 'cc'.
        $CC = findCommand( "cc" );

        # If that can't be found, try for 'gcc'.
        if ( !defined($CC) )
        {
                $CC = findCommand( "gcc" );
        }

        # This may not work if neither 'cc' or 'gcc' are
        # installed.  This is the case for Linux or Mac OS X
        # without the developer tools installed.  In that case,
        # we return undef.
        return $CC;
}





#
# @brief        Get the ps command for this OS.
#
# @return
#       the current ps command and options
#
my $PS = undef;
my $PS_PID_COLUMN = undef;
sub getCurrentPs()
{
        # Return the cached value.
        return $PS if defined( $PS );

        # Look for 'ps'.
        $PS = findCommand( "ps" );
        if ( !defined( $PS ) )
        {
                # 'ps' not found!  We may not be on a UNIX
                # derivative.  Return undef.
                return undef;
        }

        # Modify the command to add options appropriate
        # for different OSes.  Also record which column of
        # the output has the process ID.
        my $os = getCurrentOS( );
        if ( $os =~ /Darwin/i ) # Mac OS X
        {
                # PS in /bin and SysV + BSD
                # BSD-isms have added the "w" flag for wider output
                # so that process names aren't truncated to 80
                # characters (as they are on Solaris).
                # The x option is to show processes without controlling
                # terminals, such as the irodsServer.
                $PS = "$PS -exwww";
                $PS_PID_COLUMN = 0;
        }
        elsif ( $os =~ /SunOS/i ) #Solaris
        {
                # PS is in /bin and SysV
                # We specifically *do not* include -ef.  That produces
                # a wider listing, but process names are limited to 80
                # characters.  With long paths, the name of the program
                # can get truncated off the right side, making it
                # impossible to use ps() listings to find anything.
                # Instead, with "ps -e" we get only the program name,
                # but truncated to the first 8 characters.  Still bad,
                # but not as bad.
                $PS = "$PS -e";
                $PS_PID_COLUMN = 0;
        }
        elsif ( $os =~ /Linux/i )
        {
                # PS is in /bin and SysV + BSD
                $PS = "$PS -ewww";
                $PS_PID_COLUMN = 0;
        }
    elsif ( $os =~ /FreeBSD/i ) # JMC - backport 4777
    {
                   $PS = "$PS -axw";
                   $PS_PID_COLUMN = 0;
    }
        else
        {
                # Assume SysV.
                # See comment above for solaris and ps -e.
                $PS = "$PS -e";
                $PS_PID_COLUMN = 0;
        }
        return $PS;
}


#
# @brief        Get the ps command for this OS for getting PPIDs.
#
# @return
#       the current ps command and options
#
my $PS_V2 = undef;
my $PS_PID_COLUMN_V2 = undef;
my $PS_PPID_COLUMN_V2 = undef;
sub getCurrentPs_V2()
{
        # Return the cached value.
        return $PS_V2 if defined( $PS_V2 );

        # Look for 'ps'.
        $PS_V2 = findCommand( "ps" );
        if ( !defined( $PS_V2 ) )
        {
                # 'ps' not found!  We may not be on a UNIX
                # derivative.  Return undef.
                return undef;
        }

        # Modify the command to add options appropriate
        # for different OSes.  Also record which column of
        # the output has the process ID.
        my $os = getCurrentOS( );
        if ( $os =~ /Darwin/i ) # Mac OS X
        {
                # PS in /bin and SysV + BSD
                # The x option is to show processes without controlling
                # terminals, such as the irodsServer.
                $PS_V2 = "$PS_V2 -elx";
                $PS_PID_COLUMN_V2 = 1;
                $PS_PPID_COLUMN_V2 = 2;
        }
        elsif ( $os =~ /SunOS/i ) #Solaris
        {
                # PS is in /bin and SysV
                $PS_V2 = "$PS_V2 -el";
                $PS_PID_COLUMN_V2 = 3;
                $PS_PPID_COLUMN_V2 = 4;
        }
    elsif ( $os =~ /FreeBSD/i ) # JMC - backport 4777
    {
                   $PS_V2 = "$PS_V2 -alxw";
                   $PS_PID_COLUMN_V2 = 1;
                   $PS_PPID_COLUMN_V2 = 2;
    }
        elsif ( $os =~ /Linux/i )
        {
                # PS is in /bin and SysV + BSD
                $PS_V2 = "$PS_V2 -el";
                $PS_PID_COLUMN_V2 = 3;
                $PS_PPID_COLUMN_V2 = 4;
        }
        else
        {
                # Assume SysV.
                # See comment above for solaris and ps -e.
                $PS_V2 = "$PS_V2 -el";
                $PS_PID_COLUMN_V2 = 3;
                $PS_PPID_COLUMN_V2 = 4;
        }
        return $PS_V2;
}





#
# @brief        Get the column in ps output that contains the process ID.
#
# @return
#       the column number; 0 = 1st, 1 = 2nd, etc.
#
sub getCurrentPsPIDColumn()
{
        # Return the cached value.
        return $PS_PID_COLUMN if defined( $PS_PID_COLUMN );;

        # Get the right 'ps' command.  This, in turn, sets the process
        # id column.
        getCurrentPs();
        return $PS_PID_COLUMN;
}





#
# @brief        Get the current date and time.
#
# @return
#       the date and time string
#
sub getCurrentDateTime()
{
        return localtime( time( ) );
}





########################################################################
#
# Run platform programs
#

#
# @brief        Download a file via ftp.
#
# A file is downloaded to the current directory from the given
# FTP server.
#
# @param        $host
#       the FTP host
# @param        $account
#       the FTP account (usually "anonymous")
# @param        $password
#       the FTP password (usually "anonymous@")
# @param        $path
#       the FTP path of the file to download
#
# @return
#       a 3-tuple including a numeric status code, the name of the
#       local file downloaded into, and an error message on failure.
#       Status codes:
#               0  = failure (connection problem)
#               -1 = failure (login problem)
#               -2 = failure (file not found problem)
#               1  = success
#
sub ftp($$$$)
{
        my ($host,$account,$password,$path) = @_;

        # This implementation uses the Net::FTP module instead of
        # the external 'ftp' command.  This gives us predictability
        # for features and error messages.

        # Use 'passive' mode FTP.  The default "active" mode requires
        # that the remote host connect back to this host to initiate
        # a file transfer (or even an 'ls' listing).  This is a problem
        # for some corporate firewalls which require that everything
        # be initiated by the client.  "Active" mode also causes
        # problems for virtual hosts (such as running Linux within
        # Parallels on a Mac) because the host doesn't actually exist,
        # and therefore reverse name lookups, etc., may not work.
        my $ftp = Net::FTP->new( $host, "Passive" => 1 );

        my $output = "";
        if ( !defined $ftp )
        {
                # The FTP connection to the host couldn't be created.
                my $msg = $@;
                $msg =~ s/Net::FTP: //;
                $output .= "Cannot connect to download site:  $host\n";
                $output .= $msg . "\n";
                return (0,undef, $output);
        }

        if ( $ftp->login( $account, $password ) != 1 )
        {
                # The login to the FTP server failed.
                $output .= "Cannot log in to download site:  $host\n";
                $output .= $ftp->message;
                $ftp->quit( );
                return (-1,undef, $output);
        }

        # Download in binary mode.
        $ftp->binary( );

        # Download.
        my $filename = $ftp->get( $path );
        if ( !defined( $filename ) )
        {
                # The download failed.
                $output .= "Cannot download file:  $path\n";
                $output .= $ftp->message;
                $ftp->quit( );
                return (-2,undef, $output);
        }

        $ftp->quit( );
        return (1,$filename, $output);
}





#
# @brief        Run an external command.
#
# Run the command and return it's status and output.
# By routing command execution through this function,
# we can log each command, and use a portable way to
# grab the command's output.
#
# @param        $command
#       the command to run
# @return
#       a 2-tuple including a numeric exit code and the
#       stdout/stderr output of the command.
#
sub run($)
{
        my ($command) = @_;

        # Log it.  If no log is open, this does nothing.
        if ( defined( printLog ) )
        {
                printLog( "    Run:  $command\n" );
        }

        # Run it, capturing all output.
        my $output = `$command 2>&1`;

        return (($?>>8),$output);
}





#
# @brief        Uncompress a file.
#
# A compressed file is uncompressed into the current directory.
# The original file is removed.
#
# @param        $filename
#       the file to uncompress
# @return
#       a 2-tuple including a numeric exit code and the
#       stdout/stderr output of the command.
#
sub uncompress($)
{
        my ($filename) = @_;
        my $gzip = findCommand( "gzip" );
        if ( ! defined( $gzip ) )
        {
                return (1,"gzip:  command not found");
        }
        return run( "$gzip -d $filename" );
}





#
# @brief        Untar a file.
#
# A tar file is expanded into the current directory.
#
# @param        $path
#       the tar file to untar
# @return
#       a 2-tuple including a numeric exit code and the
#       stdout/stderr output of the command.
#
sub untar($)
{
        my ($path) = @_;
        my $tar = findCommand( "tar" );
        if ( ! defined( $tar ) )
        {
                return (1,"tar:  command not found");
        }
        return run( "$tar xvf $path" );
}





#
# @brief        Run a make target
#
# Make is run in the current directory with the given target.
#
# @param        $target
#       the make target
# @return
#       a 2-tuple including a numeric exit code and the
#       stdout/stderr output of the command.
#
sub make($)
{
        my ($target) = @_;
        my $make = getCurrentMake( );
        if ( ! defined( $make ) )
        {
                return (1,"make:  command not found");
        }
        return run( "$make -j 4 $target" );
}





#
# @brief        Run a ps command
#
# @param        $pattern
#       return lines matching this pattern
# @return
#       the ps output text
#
sub ps($)
{
        my ($pattern) = @_;

        my $ps = getCurrentPs( );
        if ( ! defined( $ps ) )
        {
                return undef;
        }

        my ($status,$output) = run( $ps );
        if ( $status != 0 )
        {
                # Couldn't run program.
                return undef;
        }
        my @lines = split( "\n", $output );
        my $result = "";
        foreach $line (@lines)
        {
                if ( $line =~ /$pattern/i )
                {
                        $result .= "$line\n";
                }
        }
        return $result;
}





#
# @brief        Get process IDs that match one or more patterns.
#
# Look for processes with names matching the given regular
# expression patterns.  An array of matches is returned.  The
# returned array may be larger, or smaller, than the pattern
# array, depending upon the number of matches.
#
# @param        @patterns
#       an array of patterns to look for
# @return
#       an array of process IDs
#
sub getProcessIds
{
        my (@patterns) = @_;

        my $ps = getCurrentPs( );
        if ( ! defined( $ps ) )
        {
                return undef;
        }

        # Get a process list
        my ($status,$output) = run( $ps );
        if ( $status != 0 )
        {
                # Couldn't run program.
                return undef;
        }
        my @lines = split( "\n", $output );

        # Search the results for the PIDs.
        # The column of the output that contains the PID
        # depends upon the OS and the format of its 'ps' output.
        my @pids = ();
        my $pattern;
        my $col = getCurrentPsPIDColumn( );
        foreach $pattern (@patterns)
        {
                foreach $line (@lines)
                {
                        if ( $line =~ /$pattern/i )
                        {
                                if ( $line =~ /<defunct>/ ) {
                                        # Skip Zombies (at least on Linux)
                                }
                                else {
                                        my @columns = split( " ", $line );
                                        push( @pids, $columns[$col] );
                                }
                        }
                }
        }
        return @pids;
}


#
# @brief        Get family process IDs
#
# Given a process id, find it and all it's children.
#
# @param        @pid
#       parent id to look for
# @return
#       an array of process IDs
#
sub getFamilyProcessIds
{
        my ($ppid) = @_;

        my $ps = getCurrentPs_V2( );
        if ( ! defined( $ps ) )
        {
                return undef;
        }

        # Get a process list
        my ($status,$output) = run( $ps );
        if ( $status != 0 )
        {
                # Couldn't run program.
                return undef;
        }
        my @lines = split( "\n", $output );
        # Search the results for the PIDs.
        # The column of the output that contains the PID
        # depends upon the OS and the format of its 'ps' output.
        my @pids = ();
        my $pattern;
        my $col = $PS_PID_COLUMN_V2;
        my $ppidCol = $PS_PPID_COLUMN_V2;

        foreach $line (@lines)
        {
                my @columns = split( " ", $line );
                if ( $columns[$ppidCol] =~ /$ppid/i )
                {
                        push( @pids, $columns[$col] );
                }
                if ( $columns[$col] =~ /$ppid/i )
                {
                        push( @pids, $columns[$col] );
                }
        }
        return @pids;
}





return( 1 );
