#!/bin/bash

set -e
STARTTIME="$(date +%s)"
SCRIPTNAME=`basename $0`
SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
FULLPATHSCRIPTNAME=$SCRIPTPATH/$SCRIPTNAME
COVERAGE="0"
RELEASE="0"
BUILDEIRODS="1"
COVERAGEBUILDDIR="/var/lib/e-irods"
PREFLIGHT=""
PREFLIGHTDOWNLOAD=""
PYPREFLIGHT=""

USAGE="

Usage: $SCRIPTNAME [OPTIONS] <serverType> [databaseType]
Usage: $SCRIPTNAME clean

Options:
-c      Build with coverage support (gcov)
-h      Show this help
-r      Build a release package (no debugging information, optimized)
-s      Skip compilation of E-iRODS source

Examples:
$SCRIPTNAME icat postgres
$SCRIPTNAME resource
$SCRIPTNAME -s icat postgres
$SCRIPTNAME -s resource
"

# Color Manipulation Aliases
if [[ "$TERM" == "dumb" || "$TERM" == "unknown" ]] ; then
    text_bold=""      # No Operation
    text_red=""       # No Operation
    text_green=""     # No Operation
    text_yellow=""    # No Operation
    text_blue=""      # No Operation
    text_purple=""    # No Operation
    text_cyan=""      # No Operation
    text_white=""     # No Operation
    text_reset=""     # No Operation
else
    text_bold=$(tput bold)      # Bold
    text_red=$(tput setaf 1)    # Red
    text_green=$(tput setaf 2)  # Green
    text_yellow=$(tput setaf 3) # Yellow
    text_blue=$(tput setaf 4)   # Blue
    text_purple=$(tput setaf 5) # Purple
    text_cyan=$(tput setaf 6)   # Cyan
    text_white=$(tput setaf 7)  # White
    text_reset=$(tput sgr0)     # Text Reset
fi

# boilerplate
echo "${text_cyan}${text_bold}"
echo "+------------------------------------+"
echo "| RENCI E-iRODS Build Script         |"
echo "+------------------------------------+"
date
echo "${text_reset}"

# translate long options to short
for arg
do
    delim=""
    case "$arg" in
        --coverage) args="${args}-c ";;
        --help) args="${args}-h ";;
        --release) args="${args}-r ";;
        --skip) args="${args}-s ";;
        # pass through anything else
        *) [[ "${arg:0:1}" == "-" ]] || delim="\""
        args="${args}${delim}${arg}${delim} ";;
    esac
done
# reset the translated args
eval set -- $args
# now we can process with getopts
while getopts ":chrs" opt; do
    case $opt in
        c)
        COVERAGE="1"
        echo "-c detected -- Building E-iRODS with coverage support (gcov)"
        ;;
        h)
        echo "$USAGE"
        ;;
        r)
        RELEASE="1"
        echo "-r detected -- Building a RELEASE package of E-iRODS"
        ;;
        s)
        BUILDEIRODS="0"
        echo "-s detected -- Skipping E-iRODS compilation"
        ;;
        \?)
        echo "Invalid option: -$OPTARG" >&2
        ;;
    esac
done
echo ""

# detect illogical combinations, and exit
if [ "$BUILDEIRODS" == "0" -a "$RELEASE" == "1" ] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Incompatible options:" 1>&2
    echo "      :: -s   skip compilation" 1>&2
    echo "      :: -r   build for release" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi
if [ "$BUILDEIRODS" == "0" -a "$COVERAGE" == "1" ] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Incompatible options:" 1>&2
    echo "      :: -s   skip compilation" 1>&2
    echo "      :: -c   coverage support" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi
if [ "$COVERAGE" == "1" -a "$RELEASE" == "1" ] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Incompatible options:" 1>&2
    echo "      :: -c   coverage support" 1>&2
    echo "      :: -r   build for release" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi

if [ "$COVERAGE" == "1" ] ; then
    if [ -d "$COVERAGEBUILDDIR" ] ; then
        echo "${text_red}#######################################################" 1>&2
        echo "ERROR :: $COVERAGEBUILDDIR/ already exists" 1>&2
        echo "      :: Cannot build in place with coverage enabled" 1>&2
        echo "      :: Try uninstalling the e-irods package" 1>&2
        echo "#######################################################${text_reset}" 1>&2
        exit 1
    fi
    if [ "$(id -u)" != "0" ] ; then
        echo "${text_red}#######################################################" 1>&2
        echo "ERROR :: $SCRIPTNAME must be run as root" 1>&2
        echo "      :: when building in place (coverage enabled)" 1>&2
        echo "#######################################################${text_reset}" 1>&2
        exit 1
    fi
fi



# remove options from $@
shift $((OPTIND-1))

# check arguments
if [ $# -ne 1 -a $# -ne 2 ] ; then
    echo "$USAGE" 1>&2
    exit 1
fi

# get into the correct directory
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DETECTEDDIR/../

MANDIR=man
# check for clean
if [ "$1" == "clean" ] ; then
    # clean up any build-created files
    echo "${text_green}${text_bold}Clean...${text_reset}"
    echo "Cleaning $SCRIPTNAME residuals..."
    rm -f changelog.gz
    rm -rf $MANDIR
    rm -f manual.pdf
    rm -f libe-irods.a
    rm -f plugins/resources/*.so
    rm -f plugins/resources/Makefile
    set +e
    echo "Cleaning EPM residuals..."
    cd $DETECTEDDIR/../
    rm -rf linux-2.*
    rm -rf linux-3.*
    rm -rf macosx-10.*
    rm -rf epm
    rm -rf epm*
    echo "Cleaning external residuals..."
    cd $DETECTEDDIR/../external
    rm -rf boost*
    rm -rf libarchive*
    echo "Cleaning iRODS residuals..."
    cd $DETECTEDDIR/../iRODS
    make clean > /dev/null 2>&1
    rm -rf doc/html
    rm -f server/config/reConfigs/raja1.re
    rm -f server/config/scriptMonPerf.config
    rm -f server/icat/src/icatCoreTables.sql
    rm -f server/icat/src/icatSysTables.sql
    set -e
    echo "${text_green}${text_bold}Done.${text_reset}"
    exit 0
fi


echo "${text_green}${text_bold}Detecting Build Environment${text_reset}"
GITDIR=`pwd`
BUILDDIR=$GITDIR  # we'll manipulate this later, depending on the coverage flag
cd $BUILDDIR/iRODS
echo "Detected Packaging Directory [$DETECTEDDIR]"
echo "Build Directory set to [$BUILDDIR]"
# detect operating system
DETECTEDOS=`../packaging/find_os.sh`
echo "Detected OS [$DETECTEDOS]"
DETECTEDOSVERSION=`../packaging/find_os_version.sh`
echo "Detected OS Version [$DETECTEDOSVERSION]"

if [[ $1 != "icat" && $1 != "resource" ]] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Invalid serverType [$1]" 1>&2
    echo "      :: Only 'icat' or 'resource' available at this time" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi

if [ "$1" == "icat" ] ; then
    #  if [ "$2" != "postgres" -a "$2" != "mysql" ]
    if [ "$2" != "postgres" ] ; then
        echo "${text_red}#######################################################" 1>&2
        echo "ERROR :: Invalid iCAT databaseType [$2]" 1>&2
        echo "      :: Only 'postgres' available at this time" 1>&2
        echo "#######################################################${text_reset}" 1>&2
        exit 1
    fi
fi

if [ "$DETECTEDOS" == "Ubuntu" ] ; then
    if [ "$(id -u)" != "0" ] ; then
        echo "${text_red}#######################################################" 1>&2
        echo "ERROR :: $SCRIPTNAME must be run as root" 1>&2
        echo "      :: because dpkg demands to be run as root" 1>&2
        echo "#######################################################${text_reset}" 1>&2
        exit 1
    fi
fi


################################################################################
# use error codes to determine dependencies
# does not work on solaris ('which' returns 0, regardless), so check the output as well
set +e

GPLUSPLUS=`which g++`
if [[ "$?" != "0" || `echo $GPLUSPLUS | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT g++ make"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT gcc-c++ make"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT gcc-c++ make"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT gcc4g++ gmake"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        PREFLIGHT="$PREFLIGHT homebrew/versions/gcc45"
        # mac comes with make preinstalled
    fi
fi

if [ $1 == "icat" ] ; then
    UNIXODBCDEV=`find /opt/csw/include/ /usr/include /usr/local -name sql.h 2> /dev/null`
    if [ "$UNIXODBCDEV" == "" ] ; then
        if [ "$DETECTEDOS" == "Ubuntu" ] ; then
            PREFLIGHT="$PREFLIGHT unixodbc-dev"
        elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
            PREFLIGHT="$PREFLIGHT unixODBC-devel"
        elif [ "$DETECTEDOS" == "SuSE" ] ; then
            PREFLIGHT="$PREFLIGHT unixODBC-devel"
        elif [ "$DETECTEDOS" == "Solaris" ] ; then
            PREFLIGHT="$PREFLIGHT unixodbc_dev"
        elif [ "$DETECTEDOS" == "MacOSX" ] ; then
            PREFLIGHT="$PREFLIGHT unixodbc" # not confirmed as successful
        else
            PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.unixodbc.org/download.html"
        fi
    else
        echo "Detected unixODBC-dev library [$UNIXODBCDEV]"
    fi
fi

if [[ "$DETECTEDOS" == "RedHatCompatible" || "$DETECTEDOS" == "SuSE" ]] ; then
    PYTHONDEV=`find /usr -name Python.h 2> /dev/null`
    if [[ "$PYTHONDEV" == "" ]] ; then
        if [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
            PREFLIGHT="$PREFLIGHT python-devel"
        elif [ "$DETECTEDOS" == "SuSE" ] ; then
            PREFLIGHT="$PREFLIGHT python-devel"
        fi
    fi
    RPMBUILD=`which rpmbuild`
    if [[ "$?" != "0" || `echo $RPMBUILD | awk '{print $1}'` == "no" ]] ; then
        if [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
            PREFLIGHT="$PREFLIGHT rpm-build"
        elif [ "$DETECTEDOS" == "SuSE" ] ; then
            PREFLIGHT="$PREFLIGHT rpm-build"
       fi
    fi
fi

WGET=`which wget`
if [[ "$?" != "0" || `echo $WGET | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT wget"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT wget"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT wget"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT wget"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        PREFLIGHT="$PREFLIGHT wget"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.gnu.org/software/wget/"
    fi
else
    WGETVERSION=`wget --version | head -n1 | awk '{print $3}'`
    echo "Detected wget [$WGET] v[$WGETVERSION]"
fi

DOXYGEN=`which doxygen`
if [[ "$?" != "0" || `echo $DOXYGEN | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT doxygen"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT doxygen"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT doxygen"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT doxygen"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        PREFLIGHT="$PREFLIGHT doxygen"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://doxygen.org"
    fi
else
    DOXYGENVERSION=`doxygen --version`
    echo "Detected doxygen [$DOXYGEN] v[$DOXYGENVERSION]"
fi

HELP2MAN=`which help2man`
if [[ "$?" != "0" || `echo $HELP2MAN | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        PREFLIGHT="$PREFLIGHT help2man"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.gnu.org/software/help2man/"
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      ::                http://mirrors.kernel.org/gnu/help2man/"
    fi
else
    H2MVERSION=`help2man --version | head -n1 | awk '{print $3}'`
    echo "Detected help2man [$HELP2MAN] v[$H2MVERSION]"
fi

if [ "$DETECTEDOS" == "Solaris" ] ; then
    GREPCMD="ggrep"
else
    GREPCMD="grep"
fi

#BOOST=`$GREPCMD -r "#define BOOST_VERSION " /usr/include/b* /usr/local/include/b* /opt/csw/gxx/include/b* 2> /dev/null`
#if [ "$BOOST" == "" ] ; then
#    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
#        PREFLIGHT="$PREFLIGHT libboost-all-dev"
#    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
#        PREFLIGHT="$PREFLIGHT boost-devel"
#    elif [ "$DETECTEDOS" == "SuSE" ] ; then
#        PREFLIGHT="$PREFLIGHT boost-devel"
#    elif [ "$DETECTEDOS" == "Solaris" ] ; then
#        PREFLIGHT="$PREFLIGHT boost_gcc_dev"
#        echo "      :: NOTE: pkgutil must be using 'unstable' mirror" 1>&2
#        echo "      ::       see /etc/opt/csw/pkgutil.conf" 1>&2
#    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
#        PREFLIGHT="$PREFLIGHT boost"
#    else
#        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.boost.org/users/download/"
#    fi
#else
#    BOOSTFILE=`echo $BOOST | awk -F: '{print $1}'`
#    BOOSTVERSION=`echo $BOOST | awk '{print $3}'`
#    echo "Detected BOOST libraries [$BOOSTFILE] v[$BOOSTVERSION]"
#fi

BZIP2DEV=`find /usr -name bzlib.h 2> /dev/null`
if [ "$BZIP2DEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT libbz2-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT bzip2-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT libbz2-devel"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT libbz2_dev"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.bzip.org/downloads.html"
    fi
else
    echo "Detected bzip2 library [$BZIP2DEV]"
fi

ZLIBDEV=`find /usr/include -name zlib.h 2> /dev/null`
if [ "$ZLIBDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT zlib1g-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT zlib-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT zlib-devel"
    # Solaris comes with SUNWzlib which provides /usr/include/zlib.h
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://zlib.net/"
    fi
else
    echo "Detected zlib library [$ZLIBDEV]"
fi


#FINDLIBARCHIVE=`../packaging/find_so.sh libarchive.so 2> /dev/null`
#FINDLIBARCHIVEH=`find /usr -name archive.h 2> /dev/null`
#if [[ "$FINDLIBARCHIVE" == "FAIL" || "$FINDLIBARCHIVEH" == "" ]] ; then
#    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
#        if [ "$DETECTEDOSVERSION" \< "11" ]; then
#            PREFLIGHT="$PREFLIGHT libarchive1 libarchive-dev"
#        else
#            PREFLIGHT="$PREFLIGHT libarchive12 libarchive-dev"
#        fi
#    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
#        PREFLIGHT="$PREFLIGHT libarchive libarchive-devel"
#    elif [ "$DETECTEDOS" == "SuSE" ] ; then
#        PREFLIGHT="$PREFLIGHT libarchive2 libarchive-devel"
#    elif [ "$DETECTEDOS" == "Solaris" ] ; then
#        PREFLIGHT="$PREFLIGHT libarchive2 libarchive_dev"
#    # MacOSX is distributed with libarchive - in /usr/lib
#    else
#        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://libarchive.github.com/"
#    fi
#else
#    echo "Detected libarchive library [$FINDLIBARCHIVE]"
#fi

OPENSSLDEV=`find /usr/include/openssl /opt/csw/include/openssl -name sha.h 2> /dev/null`
if [ "$OPENSSLDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT libssl-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT openssl-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT libopenssl-devel"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT libssl_dev"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.openssl.org/source/"
    fi
else
    echo "Detected OpenSSL sha.h library [$OPENSSLDEV]"
fi

FINDPOSTGRESBIN=`../packaging/find_postgres_bin.sh 2> /dev/null`
if [ "$FINDPOSTGRESBIN" == "FAIL" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT postgresql"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT postgresql"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT postgresql"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT postgresql_dev"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        PREFLIGHT="$PREFLIGHT postgresql"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.postgresql.org/download/"
    fi
else
    echo "Detected PostgreSQL binary [$FINDPOSTGRESBIN]"
fi

EASYINSTALL=`which easy_install`
if [[ "$?" != "0" || `echo $EASYINSTALL | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT python-setuptools"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT python-setuptools python-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT python-setuptools"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT pysetuptools"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        PREFLIGHT="$PREFLIGHT"
        # should have distribute included already
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://pypi.python.org/pypi/setuptools/"
    fi
else
    echo "Detected easy_install [$EASYINSTALL]"
fi


# check python package prerequisites
RST2PDF=`which rst2pdf`
if [[ "$?" != "0" || `echo $RST2PDF | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        PREFLIGHT="$PREFLIGHT rst2pdf"
    else
        PYPREFLIGHT="$PYPREFLIGHT rst2pdf"
    fi
else
    RST2PDFVERSION=`rst2pdf --version`
    echo "Detected rst2pdf [$RST2PDF] v[$RST2PDFVERSION]"
fi

# print out prerequisites error
if [ "$PREFLIGHT" != "" ] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: $SCRIPTNAME requires some software to be installed" 1>&2
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then
        echo "      :: try: ${text_reset}sudo apt-get install$PREFLIGHT${text_red}" 1>&2
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        echo "      :: try: ${text_reset}sudo yum install$PREFLIGHT${text_red}" 1>&2
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        echo "      :: try: ${text_reset}sudo zypper install$PREFLIGHT${text_red}" 1>&2
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        echo "      :: try: ${text_reset}sudo pkgutil --install$PREFLIGHT${text_red}" 1>&2
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        echo "      :: try: ${text_reset}brew install$PREFLIGHT${text_red}" 1>&2
    else
        echo "      :: NOT A DETECTED OPERATING SYSTEM" 1>&2
    fi
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi

if [ "$PREFLIGHTDOWNLOAD" != "" ] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: $SCRIPTNAME requires some software to be installed" 1>&2
    echo "$PREFLIGHTDOWNLOAD" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi

ROMAN=`python -c "import roman"`
if [ "$?" != "0" ] ; then
    PYPREFLIGHT="$PYPREFLIGHT roman"
else
    ROMANLOCATION=`python -c "import roman; print roman.__file__"` # expecting ".../roman.pyc"
    echo "Detected python module 'roman' [$ROMANLOCATION]"
fi

# print out python prerequisites error
if [ "$PYPREFLIGHT" != "" ] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: python requires some software to be installed" 1>&2
    echo "      :: try: ${text_reset}sudo easy_install$PYPREFLIGHT${text_red}" 1>&2
    echo "      ::   (easy_install provided by pysetuptools or pydistribute)" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi

# reset to exit on an error
set -e

################################################################################

# find number of cpus
if [ "$DETECTEDOS" == "MacOSX" ] ; then
    DETECTEDCPUCOUNT=`sysctl -n hw.ncpu`
elif [ "$DETECTEDOS" == "Solaris" ] ; then
    DETECTEDCPUCOUNT=`/usr/sbin/psrinfo -p`
else
    DETECTEDCPUCOUNT=`cat /proc/cpuinfo | grep processor | wc -l`
fi
if [ "$DETECTEDCPUCOUNT" \< "2" ] ; then
    DETECTEDCPUCOUNT=1
fi
CPUCOUNT=$(( $DETECTEDCPUCOUNT + 3 ))
MAKEJCMD="make -j $CPUCOUNT"

# print out detected CPU information
echo "${text_cyan}${text_bold}-------------------------------------"
echo "Detected CPUs:    $DETECTEDCPUCOUNT"
echo "Compiling with:   $MAKEJCMD"
echo "-------------------------------------${text_reset}"
sleep 1

################################################################################

# LOCAL COMPILATIONS - in ./external
if [ "$BUILDEIRODS" == "1" ] ; then

    # make sure ./external exists
    mkdir -p $BUILDDIR/external

    # build a copy of libarchive
    EIRODS_BUILD_LIBARCHIVEVERSION="libarchive-3.0.4"
    cd $BUILDDIR/external/
    if [ -d "$EIRODS_BUILD_LIBARCHIVEVERSION" ] ; then
        echo "${text_green}${text_bold}Detected copy of [$EIRODS_BUILD_LIBARCHIVEVERSION]${text_reset}"
    else
        echo "${text_green}${text_bold}Downloading [$EIRODS_BUILD_LIBARCHIVEVERSION] from github.com${text_reset}"
        wget https://github.com/downloads/libarchive/libarchive/$EIRODS_BUILD_LIBARCHIVEVERSION.tar.gz
        gunzip $EIRODS_BUILD_LIBARCHIVEVERSION.tar.gz
        tar xf $EIRODS_BUILD_LIBARCHIVEVERSION.tar
    fi
    echo "${text_green}${text_bold}Building [$EIRODS_BUILD_LIBARCHIVEVERSION]${text_reset}"
    cd $BUILDDIR/external/$EIRODS_BUILD_LIBARCHIVEVERSION
    if [ ! -e ".libs/libarchive.a" ] ; then
        CFLAGS="-fPIC" ./configure
    fi
    $MAKEJCMD

    # build a copy of boost
    EIRODS_BUILD_BOOSTVERSION="boost_1_52_0"
    cd $BUILDDIR/external/
    if [ -d "$EIRODS_BUILD_BOOSTVERSION" ] ; then
        echo "${text_green}${text_bold}Detected copy of [$EIRODS_BUILD_BOOSTVERSION]${text_reset}"
    else
        echo "${text_green}${text_bold}Downloading [$EIRODS_BUILD_BOOSTVERSION] from sourceforge.net${text_reset}"
        wget -O $EIRODS_BUILD_BOOSTVERSION.tar.gz http://sourceforge.net/projects/boost/files/boost/1.52.0/$EIRODS_BUILD_BOOSTVERSION.tar.gz/download
        gunzip $EIRODS_BUILD_BOOSTVERSION.tar.gz
        tar xf $EIRODS_BUILD_BOOSTVERSION.tar
        echo "Finished exploding tarfile"
    fi
    echo "${text_green}${text_bold}Building [$EIRODS_BUILD_BOOSTVERSION]${text_reset}"
    cd $BUILDDIR/external/$EIRODS_BUILD_BOOSTVERSION
    ./bootstrap.sh
    ./bjam link=static threading=multi cxxflags="-fPIC" -j$CPUCOUNT

fi

################################################################################

echo "-----------------------------"
echo "${text_green}${text_bold}Configuring and Building iRODS${text_reset}"
echo "-----------------------------"

# set up own temporary configfile
cd $BUILDDIR/iRODS
TMPCONFIGFILE=/tmp/$USER/irods.config.epm
mkdir -p $(dirname $TMPCONFIGFILE)



# set up variables for icat configuration
if [ $1 == "icat" ] ; then
    SERVER_TYPE="ICAT"
    DB_TYPE=$2
    EPMFILE="../packaging/irods.config.icat.epm"

    if [ "$BUILDEIRODS" == "1" ] ; then
        # =-=-=-=-=-=-=-
        # bake SQL files for different database types
        # NOTE:: icatSysInserts.sql is handled by the packager as we rely on the default zone name
        serverSqlDir="./server/icat/src"
        convertScript="$serverSqlDir/convertSql.pl"

        echo "Converting SQL: [$convertScript] [$DB_TYPE] [$serverSqlDir]"
        `perl $convertScript $2 $serverSqlDir &> /dev/null`
        if [ "$?" -ne "0" ] ; then
            echo "Failed to convert SQL forms" 1>&2
            exit 1
        fi

        # =-=-=-=-=-=-=-
        # insert postgres path into list file
        if [ "$DB_TYPE" == "postgres" ] ; then
            # need to do a dirname here, as the irods.config is expected to have a path
            # which will be appended with a /bin
            EIRODSPOSTGRESPATH=`../packaging/find_postgres_bin.sh`
            if [ "$EIRODSPOSTGRESPATH" == "FAIL" ] ; then
                exit 1
            fi
            EIRODSPOSTGRESPATH=`dirname $EIRODSPOSTGRESPATH`
            EIRODSPOSTGRESPATH="$EIRODSPOSTGRESPATH/"

            echo "Detected PostgreSQL path [$EIRODSPOSTGRESPATH]"
            sed -e s,EIRODSPOSTGRESPATH,$EIRODSPOSTGRESPATH, $EPMFILE > $TMPCONFIGFILE
        else
            echo "TODO: irods.config for DBTYPE other than postgres"
        fi
    fi
    # set up variables for resource configuration
else
    SERVER_TYPE="RESOURCE"
    EPMFILE="../packaging/irods.config.resource.epm"
    cp $EPMFILE $TMPCONFIGFILE
fi


# =-=-=-=-=-=-=-
# generate random password for database
RANDOMDBPASS=`cat /dev/urandom | base64 | head -c15`

if [ "$BUILDEIRODS" == "1" ] ; then

    if [ "$COVERAGE" == "1" ] ; then
        # change context for BUILDDIR - we're building in place for gcov linking
        BUILDDIR=$COVERAGEBUILDDIR
        echo "${text_green}${text_bold}Switching context to [$BUILDDIR] for coverage-enabled build${text_reset}"
        # copy entire local tree to real package target location
        echo "${text_green}${text_bold}Copying files into place...${text_reset}"
        cp -r $GITDIR $BUILDDIR
        # go there
        cd $BUILDDIR/iRODS
    fi

    rm -f ./config/config.mk
    rm -f ./config/platform.mk

    # =-=-=-=-=-=-=-
    # run configure to create Makefile, config.mk, platform.mk, etc.
    ./scripts/configure
    # overwrite with our values
    cp $TMPCONFIGFILE ./config/irods.config
    # run with our updated irods.config
    ./scripts/configure
    # again to reset IRODS_HOME
    cp $TMPCONFIGFILE ./config/irods.config

    # change password for database to be consistent with that within the e-irods.list file
    # for installation
    sed -e "s,TEMPLATE_DB_PASS,$RANDOMDBPASS," ./config/irods.config > /tmp/irods.config
    mv /tmp/irods.config ./config/irods.config

    # handle issue with IRODS_HOME being overwritten by the configure script
    irodsctl_irods_home=`./scripts/find_irods_home.sh`
    sed -e "\,^IRODS_HOME,s,^.*$,IRODS_HOME=$irodsctl_irods_home," ./irodsctl > /tmp/irodsctl.tmp
    mv /tmp/irodsctl.tmp ./irodsctl
    chmod 755 ./irodsctl

    # update libarchive_dir to our external path
    EIRODS_LIBARCHIVE_DIR=$BUILDDIR/external/$EIRODS_BUILD_LIBARCHIVEVERSION
    sed -e "\,^LIBARCHIVE_DIR=,s,^.*$,LIBARCHIVE_DIR=$EIRODS_LIBARCHIVE_DIR," ./config/config.mk > /tmp/eirods-config.mk
    mv /tmp/eirods-config.mk ./config/config.mk

    # update boost_dir to our external path
    EIRODS_BOOST_DIR=$BUILDDIR/external/$EIRODS_BUILD_BOOSTVERSION
    sed -e "\,^BOOST_DIR=,s,^.*$,BOOST_DIR=$EIRODS_BOOST_DIR," ./config/config.mk > /tmp/eirods-config.mk
    mv /tmp/eirods-config.mk ./config/config.mk

#    # turn on irods boost flag
#    sed -e "s,^# USE_BOOST=1,USE_BOOST=1," ./config/config.mk > /tmp/eirods-config.mk
#    mv /tmp/eirods-config.mk ./config/config.mk

    # twiddle coverage flag in platform.mk based on whether this is a coverage (gcov) build
    if [ "$COVERAGE" == "1" ] ; then
        sed -e "s,EIRODS_BUILD_COVERAGE=0,EIRODS_BUILD_COVERAGE=1," ./config/platform.mk > /tmp/eirods-platform.mk
        mv /tmp/eirods-platform.mk ./config/platform.mk
    fi

    # twiddle debug flag in platform.mk based on whether this is a release build
    if [ "$RELEASE" == "1" ] ; then
        sed -e "s,EIRODS_BUILD_DEBUG=1,EIRODS_BUILD_DEBUG=0," ./config/platform.mk > /tmp/eirods-platform.mk
        mv /tmp/eirods-platform.mk ./config/platform.mk
    fi

    # update resources Makefile to find system shared libraries
    # libz
    found_so=`../packaging/find_so.sh libz.so`
    sed -e s,SYSTEM_LIBZ_SO,$found_so, ../plugins/resources/Makefile.in > /tmp/eirods_p_r_Makefile
    mv /tmp/eirods_p_r_Makefile ../plugins/resources/Makefile
    # bzip2
    found_so=`../packaging/find_so.sh libbz2.so`
    sed -e s,SYSTEM_LIBBZ2_SO,$found_so, ../plugins/resources/Makefile > /tmp/eirods_p_r_Makefile
    mv /tmp/eirods_p_r_Makefile ../plugins/resources/Makefile
#    # libarchive
#    found_so=`../packaging/find_so.sh libarchive.so`
#    sed -e s,SYSTEM_LIBARCHIVE_SO,$found_so, ../plugins/resources/Makefile > /tmp/eirods_p_r_Makefile
#    mv /tmp/eirods_p_r_Makefile ../plugins/resources/Makefile
#    # boost filesystem
#    found_so=`../packaging/find_so.sh libboost_filesystem.so`
#    sed -e s,SYSTEM_LIBBOOST_FILESYSTEM_SO,$found_so, ../plugins/resources/Makefile > /tmp/eirods_p_r_Makefile
#    mv /tmp/eirods_p_r_Makefile ../plugins/resources/Makefile
#    # boost system
#    found_so=`../packaging/find_so.sh libboost_system.so`
#    sed -e s,SYSTEM_LIBBOOST_SYSTEM_SO,$found_so, ../plugins/resources/Makefile > /tmp/eirods_p_r_Makefile
#    mv /tmp/eirods_p_r_Makefile ../plugins/resources/Makefile

    # =-=-=-=-=-=-=-
    # modify the eirods_ms_home.h file with the proper path to the binary directory
    irods_msvc_home=`./scripts/find_irods_home.sh`
    irods_msvc_home="$irods_msvc_home/server/bin/"
    sed -e s,EIRODSMSVCPATH,$irods_msvc_home, ./server/re/include/eirods_ms_home.h.src > /tmp/eirods_ms_home.h
    mv /tmp/eirods_ms_home.h ./server/re/include/


    ###########################################
    # single 'make' time on an 8 core machine
    ###########################################
    #        time make           1m55.508s
    #        time make -j 1      1m55.023s
    #        time make -j 2      0m17.199s
    #        time make -j 3      0m11.873s
    #        time make -j 4      0m9.894s   <-- inflection point
    #        time make -j 5      0m9.164s
    #        time make -j 6      0m8.515s
    #        time make -j 7      0m8.042s
    #        time make -j 8      0m7.898s
    #        time make -j 9      0m7.911s
    #        time make -j 10     0m7.898s
    #        time make -j        0m30.920s
    ###########################################
    # single 'make' time on a single core VM
    ###########################################
    #        time make           3m1.410s
    #        time make -j 2      2m13.481s
    #        time make -j 4      1m52.533s
    #        time make -j 5      1m48.611s
    ###########################################
    set +e
    $MAKEJCMD
    set -e
    $MAKEJCMD
    if [ "$?" != "0" ] ; then
        exit 1
    fi

    # =-=-=-=-=-=-=-
    # build resource plugins

	cd $BUILDDIR/plugins/resources/
	make
	cd $BUILDDIR

    # =-=-=-=-=-=-=-
    # update EPM list template with values from irods.config
    cd $BUILDDIR
    #   database name
    NEW_DB_NAME=`awk -F\' '/^\\$DB_NAME/ {print $2}' iRODS/config/irods.config`
    sed -e "s,TEMPLATE_DB_NAME,$NEW_DB_NAME," ./packaging/e-irods.list.template > /tmp/eirodslist.tmp
    mv /tmp/eirodslist.tmp ./packaging/e-irods.list
#    #   database admin role
#    NEW_DB_ADMIN_ROLE=`awk -F\' '/^\\$DB_ADMIN_NAME/ {print $2}' iRODS/config/irods.config`
#    sed -e "s,TEMPLATE_DB_ADMIN_ROLE,$NEW_DB_ADMIN_ROLE," ./packaging/e-irods.list > /tmp/eirodslist.tmp
#    mv /tmp/eirodslist.tmp ./packaging/e-irods.list
    #   database type
    NEW_DB_TYPE=`awk -F\' '/^\\$DATABASE_TYPE/ {print $2}' iRODS/config/irods.config`
    sed -e "s,TEMPLATE_DB_TYPE,$NEW_DB_TYPE," ./packaging/e-irods.list > /tmp/eirodslist.tmp
    mv /tmp/eirodslist.tmp ./packaging/e-irods.list
    #   database host
    NEW_DB_HOST=`awk -F\' '/^\\$DATABASE_HOST/ {print $2}' iRODS/config/irods.config`
    sed -e "s,TEMPLATE_DB_HOST,$NEW_DB_HOST," ./packaging/e-irods.list > /tmp/eirodslist.tmp
    mv /tmp/eirodslist.tmp ./packaging/e-irods.list
    #   database port
    NEW_DB_PORT=`awk -F\' '/^\\$DATABASE_PORT/ {print $2}' iRODS/config/irods.config`
    sed -e "s,TEMPLATE_DB_PORT,$NEW_DB_PORT," ./packaging/e-irods.list > /tmp/eirodslist.tmp
    mv /tmp/eirodslist.tmp ./packaging/e-irods.list
    #   database password
    sed -e "s,TEMPLATE_DB_PASS,$RANDOMDBPASS," ./packaging/e-irods.list > /tmp/eirodslist.tmp
    mv /tmp/eirodslist.tmp ./packaging/e-irods.list


    set +e
    # generate manual in pdf format
    echo "${text_green}${text_bold}Building E-iRODS Manual${text_reset}"
    cd $BUILDDIR
    rst2pdf manual.rst -o manual.pdf
    if [ "$?" != "0" ] ; then
        echo "${text_red}#######################################################" 1>&2
        echo "ERROR :: Failed generating manual.pdf" 1>&2
        echo "#######################################################${text_reset}" 1>&2
        exit 1
    fi
    # generate doxygen for microservices
    echo "${text_green}${text_bold}Building E-iRODS Doxygen Output${text_reset}"
    cd $BUILDDIR/iRODS
    doxygen ./config/doxygen-saved.cfg
    if [ "$?" != "0" ] ; then
        echo "${text_red}#######################################################" 1>&2
        echo "ERROR :: Failed generating doxygen output" 1>&2
        echo "#######################################################${text_reset}" 1>&2
        exit 1
    fi
    set -e

    # generate tgz file for inclusion in coverage package
    if [ "$COVERAGE" == "1" ] ; then
        set +e
        GCOVFILELIST="gcovfilelist.txt"
        GCOVFILENAME="gcovfiles.tgz"
        cd $BUILDDIR
        find ./iRODS -name "*.h" -o -name "*.c" -o -name "*.gcno" > $GCOVFILELIST
        tar czf $GCOVFILENAME -T $GCOVFILELIST
        ls -al $GCOVFILELIST
        ls -al $GCOVFILENAME
        set -e
    fi

    # generate development package archive file
    if [ "$RELEASE" == "1" ] ; then
        echo "${text_green}${text_bold}Building development package archive file...${text_reset}"
        cd $BUILDDIR
        ./packaging/make_e-irods_dev_archive.sh
    fi


fi # if $BUILDEIRODS


# prepare changelog for various platforms
cd $BUILDDIR
gzip -9 -c changelog > changelog.gz


# prepare man pages for the icommands
cd $BUILDDIR
rm -rf $MANDIR
mkdir -p $MANDIR
if [ "$H2MVERSION" \< "1.37" ] ; then
    echo "NOTE :: Skipping man page generation -- help2man version needs to be >= 1.37"
    echo "     :: (or, add --version capability to all iCommands)"
    echo "     :: (installed here: help2man version $H2MVERSION)"
else
    EIRODSMANVERSION=`grep "^%version" ./packaging/e-irods.list | awk '{print $2}'`
    ICMDDIR="iRODS/clients/icommands/bin"
    ICMDS=(
    genOSAuth     
    iadmin        
    ibun          
    icd           
    ichksum       
    ichmod        
    icp           
    idbo          
    idbug         
    ienv          
    ierror        
    iexecmd       
    iexit         
    ifsck         
    iget          
    igetwild      
    igroupadmin   
    ihelp         
    iinit         
    ilocate       
    ils           
    ilsresc       
    imcoll        
    imeta         
    imiscsvrinfo  
    imkdir        
    imv           
    ipasswd       
    iphybun       
    iphymv        
    ips           
    iput          
    ipwd          
    iqdel         
    iqmod         
    iqstat        
    iquest        
    iquota        
    ireg          
    irepl         
    irm           
    irmtrash      
    irsync        
    irule         
    iscan         
    isysmeta      
    itrim         
    iuserinfo     
    ixmsg         
    )
    for ICMD in "${ICMDS[@]}"
    do
        help2man -h -h -N -n "an E-iRODS iCommand" --version-string="E-iRODS-$EIRODSMANVERSION" $ICMDDIR/$ICMD > $MANDIR/$ICMD.1
    done
    for manfile in `ls $MANDIR`
    do
        gzip -9 $MANDIR/$manfile
    done
fi





# run EPM for package type of this machine
# available from: http://fossies.org/unix/privat/epm-4.2-source.tar.gz
# md5sum 3805b1377f910699c4914ef96b273943

if [ "$BUILDEIRODS" == "1" ] ; then
    # get RENCI updates to EPM from repository
    echo "${text_green}${text_bold}Downloading EPM from RENCI${text_reset}"
    cd $BUILDDIR
    RENCIEPM="epm42-renci.tar.gz"
    rm -rf epm
    rm -f $RENCIEPM
    wget ftp://ftp.renci.org/pub/e-irods/build/$RENCIEPM
    tar -xf $RENCIEPM
    # configure
    echo "${text_green}${text_bold}Configuring EPM${text_reset}"
    cd $BUILDDIR/epm
    set +e
    ./configure > /dev/null
    if [ "$?" != "0" ] ; then
        exit 1
    fi
    # build
    echo "${text_green}${text_bold}Building EPM${text_reset}"
    make > /dev/null
    if [ "$?" != "0" ] ; then
        exit 1
    fi
    set -e
fi

if [ "$COVERAGE" == "1" ] ; then
    # sets EPM to not strip binaries of debugging information
    EPMOPTS="-g"
    # sets listfile coverage options
    EPMOPTS="$EPMOPTS COVERAGE=true"
else
    EPMOPTS=""
fi

cd $BUILDDIR
unamem=`uname -m`
if [[ "$unamem" == "x86_64" || "$unamem" == "amd64" ]] ; then
    arch="amd64"
else
    arch="i386"
fi
if [ "$DETECTEDOS" == "RedHatCompatible" ] ; then # CentOS and RHEL and Fedora
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS RPMs${text_reset}"
    epmvar="REDHATRPM$SERVER_TYPE"
    ostype=`awk '{print $1}' /etc/redhat-release`
    osversion=`awk '{print $3}' /etc/redhat-release`
    if [ "$ostype" == "CentOS" -a "$osversion" \> "6" ]; then
        epmosversion="CENTOS6"
    else
        epmosversion="NOTCENTOS6"
    fi
    ./epm/epm $EPMOPTS -f rpm e-irods $epmvar=true $epmosversion=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -f rpm e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -f rpm e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
elif [ "$DETECTEDOS" == "SuSE" ] ; then # SuSE
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS RPMs${text_reset}"
    epmvar="SUSERPM$SERVER_TYPE"
    ./epm/epm $EPMOPTS -f rpm e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -f rpm e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -f rpm e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
elif [ "$DETECTEDOS" == "Ubuntu" ] ; then  # Ubuntu
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DEBs${text_reset}"
    epmvar="DEB$SERVER_TYPE"
    ./epm/epm $EPMOPTS -a $arch -f deb e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -a $arch -f deb e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -a $arch -f deb e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
elif [ "$DETECTEDOS" == "Solaris" ] ; then  # Solaris
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS PKGs${text_reset}"
    epmvar="PKG$SERVER_TYPE"
    ./epm/epm $EPMOPTS -f pkg e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -f pkg e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -f pkg e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
elif [ "$DETECTEDOS" == "MacOSX" ] ; then  # MacOSX
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DMGs${text_reset}"
    epmvar="OSX$SERVER_TYPE"
    ./epm/epm $EPMOPTS -f osx e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -f osx e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -f osx e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
else
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Unknown OS, cannot generate packages with EPM" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi


if [ "$COVERAGE" == "1" ] ; then
    # copy important bits back up
    echo "${text_green}${text_bold}Copying generated packages back to original working directory...${text_reset}"
    if [ "$DETECTEDOS" == "RedHatCompatible" ] ; then EXTENSION="rpm"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then EXTENSION="rpm"
    elif [ "$DETECTEDOS" == "Ubuntu" ] ; then EXTENSION="deb"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then EXTENSION="pkg"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then EXTENSION="dmg"
    fi
    # get packages
    for f in `find . -name "*.$EXTENSION"` ; do mkdir -p $GITDIR/`dirname $f`; cp $f $GITDIR/$f; done
    # delete target build directory, so a package install can go there
    cd $GITDIR
    rm -rf $COVERAGEBUILDDIR
fi

# grant write permission to all, in case this was run via sudo
cd $GITDIR
chmod -R a+w .

# boilerplate
TOTALTIME="$(($(date +%s)-STARTTIME))"
echo "${text_cyan}${text_bold}"
echo "+------------------------------------+"
echo "| RENCI E-iRODS Build Script         |"
echo "|                                    |"
printf "|   Completed in %02dm%02ds              |\n" "$((TOTALTIME/60))" "$((TOTALTIME%60))"
echo "+------------------------------------+"
echo "${text_reset}"
