#!/bin/bash

set -e
STARTTIME="$(date +%s)"
SCRIPTNAME=`basename $0`
SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
FULLPATHSCRIPTNAME=$SCRIPTPATH/$SCRIPTNAME
COVERAGE="0"
RELEASE="0"
BUILDIRODS="1"
PORTABLE="0"
COVERAGEBUILDDIR="/var/lib/irods"
PREFLIGHT=""
PREFLIGHTDOWNLOAD=""
PYPREFLIGHT=""
IRODSPACKAGEDIR="./build"

USAGE="

Usage: $SCRIPTNAME [OPTIONS] <serverType> [databaseType]
Usage: $SCRIPTNAME docs
Usage: $SCRIPTNAME clean

Options:
-c      Build with coverage support (gcov)
-h      Show this help
-r      Build a release package (no debugging information, optimized)
-s      Skip compilation of iRODS source
-p      Portable option, ignores OS and builds a tar.gz

Long Options:
--run-in-place    Build server for in-place execution (not recommended)

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
echo "| RENCI iRODS Build Script           |"
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
        --portable) args="${args}-p ";;
        --run-in-place) args="${args}-z ";;
        # pass through anything else
        *) [[ "${arg:0:1}" == "-" ]] || delim="\""
        args="${args}${delim}${arg}${delim} ";;
    esac
done
# reset the translated args
eval set -- $args
# now we can process with getopts
while getopts ":chrspz" opt; do
    case $opt in
        c)
        COVERAGE="1"
        TARGET=$2
        echo "-c detected -- Building iRODS with coverage support (gcov)"
        echo "${text_green}${text_bold}TARGET=[$TARGET]${text_reset}"
        if [ "$TARGET" == "icat" ] ; then
            echo "${text_green}${text_bold}TARGET is ICAT${text_reset}"
        fi
        ;;
        h)
        echo "$USAGE"
        ;;
        r)
        RELEASE="1"
        echo "-r detected -- Building a RELEASE package of iRODS"
        ;;
        s)
        BUILDIRODS="0"
        echo "-s detected -- Skipping iRODS compilation"
        ;;
        p)
        PORTABLE="1"
        echo "-p detected -- Building portable package"
        ;;
        z)
        RUNINPLACE="1"
        echo "--run-in-place detected -- Building for in-place execution"
        ;;
        \?)
        echo "Invalid option: -$OPTARG" >&2
        ;;
    esac
done
echo ""

# detect illogical combinations, and exit
if [ "$BUILDIRODS" == "0" -a "$RELEASE" == "1" ] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Incompatible options:" 1>&2
    echo "      :: -s   skip compilation" 1>&2
    echo "      :: -r   build for release" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi
if [ "$BUILDIRODS" == "0" -a "$COVERAGE" == "1" ] ; then
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
        echo "      :: Try uninstalling the irods package" 1>&2
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

# begin self-awareness
echo "${text_green}${text_bold}Detecting Build Environment${text_reset}"
echo "Detected Packaging Directory [$DETECTEDDIR]"
GITDIR=`pwd`
BUILDDIR=$GITDIR  # we'll manipulate this later, depending on the coverage flag
EPMCMD="external/epm/epm"
cd $BUILDDIR/iRODS
echo "Build Directory set to [$BUILDDIR]"
# read iRODS Version from file
source ../VERSION
echo "Detected iRODS Version to Build [$IRODSVERSION]"
echo "Detected iRODS Version Integer [$IRODSVERSIONINT]"
# detect operating system
DETECTEDOS=`../packaging/find_os.sh`
if [ "$PORTABLE" == "1" ] ; then
  DETECTEDOS="Portable"
fi
echo "Detected OS [$DETECTEDOS]"
DETECTEDOSVERSION=`../packaging/find_os_version.sh`
echo "Detected OS Version [$DETECTEDOSVERSION]"




############################################################
# FUNCTIONS
############################################################

# creates a timestamped tempfile for quick usage
set_tmpfile() {
  mkdir -p /tmp/$USER
  TMPFILE=/tmp/$USER/$(date "+%Y%m%d-%H%M%S.%N.irods.tmp")
}

# find number of cpus
detect_number_of_cpus_and_set_makejcmd() {
    DETECTEDCPUCOUNT=`$BUILDDIR/packaging/get_cpu_count.sh`
    CPUCOUNT=$(( $DETECTEDCPUCOUNT + 3 ))
    MAKEJCMD="make -j $CPUCOUNT"

    # print out detected CPU information
    echo "${text_cyan}${text_bold}-------------------------------------"
    echo "Detected CPUs:    $DETECTEDCPUCOUNT"
    echo "Compiling with:   $MAKEJCMD"
    echo "-------------------------------------${text_reset}"
    sleep 1
}

# rename generated packages appropriately
rename_generated_packages() {

    # parameters
    if [ "$1" == "" ] ; then
        echo "rename_generated_packages() expected 1 parameter"
        exit 1
    fi
    TARGET=$1

    #################
    # extensions
    cd $BUILDDIR
    SUFFIX=""
    if   [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
	EXTENSION="rpm"
	SUFFIX="-redhat"
	if [ "$epmosversion" == "CENTOS6" ] ; then
            SUFFIX="-centos6"
	fi
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
	EXTENSION="rpm"
	SUFFIX="-suse"
    elif [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
	EXTENSION="deb"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
	EXTENSION="pkg"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
	EXTENSION="dmg"
    elif [ "$DETECTEDOS" == "ArchLinux" ] ; then
	EXTENSION="tar.gz"
    elif [ "$DETECTEDOS" == "Portable" ] ; then
	EXTENSION="tar.gz"
    fi

    #################
    # icat and resource server packages
    RENAME_SOURCE="./linux*/irods-*$IRODSVERSION*.$EXTENSION"
    RENAME_SOURCE_DOCS=${RENAME_SOURCE/irods-/irods-docs-}
    RENAME_SOURCE_DEV=${RENAME_SOURCE/irods-/irods-dev-}
    RENAME_SOURCE_RUNTIME=${RENAME_SOURCE/irods-/irods-runtime-}
    RENAME_SOURCE_ICOMMANDS=${RENAME_SOURCE/irods-/irods-icommands-}
    SOURCELIST=`ls $RENAME_SOURCE`
    echo "EPM produced packages:"
    echo "$SOURCELIST"
    # prepare target build directory
    mkdir -p $IRODSPACKAGEDIR
    # vanilla construct
    RENAME_DESTINATION="$IRODSPACKAGEDIR/irods-$IRODSVERSION-64bit.$EXTENSION"
    # docs build
    RENAME_DESTINATION_DOCS=${RENAME_DESTINATION/irods-/irods-docs-}
    # add OS-specific suffix
    if [ "$SUFFIX" != "" ] ; then
	RENAME_DESTINATION=${RENAME_DESTINATION/.$EXTENSION/$SUFFIX.$EXTENSION}
    fi
    # release build (also building icommands)
    RENAME_DESTINATION_DEV=${RENAME_DESTINATION/irods-/irods-dev-}
    RENAME_DESTINATION_RUNTIME=${RENAME_DESTINATION/irods-/irods-runtime-}
    RENAME_DESTINATION_ICOMMANDS=${RENAME_DESTINATION/irods-/irods-icommands-}
    # icat or resource
    if [ "$TARGET" == "icat" ] ; then
        RENAME_DESTINATION=${RENAME_DESTINATION/irods-/irods-icat-}
    elif [ "$TARGET" == "resource" ] ; then
        RENAME_DESTINATION=${RENAME_DESTINATION/irods-/irods-resource-}
    fi
    # coverage build
    if [ "$COVERAGE" == "1" ] ; then
        RENAME_DESTINATION=${RENAME_DESTINATION/-64bit/-64bit-coverage}
        RENAME_DESTINATION_DEV=${RENAME_DESTINATION_DEV/-64bit/-64bit-coverage}
    fi

    #################
    # database packages
    if [ "$TARGET" == "icat" ] ; then
        DB_SOURCE="./plugins/database/linux*/*database*.$EXTENSION"
        echo `ls $DB_SOURCE`
        DB_PACKAGE=`basename $DB_SOURCE`
        DB_DESTINATION="$IRODSPACKAGEDIR/$DB_PACKAGE"
        DB_DESTINATION=`echo $DB_DESTINATION | sed -e "s,\\(-[^-]*\\)\{3\}\\.$EXTENSION\$,.$EXTENSION,"`
        # add OS-specific suffix
        if [ "$SUFFIX" != "" ] ; then
            DB_DESTINATION=${DB_DESTINATION/.$EXTENSION/$SUFFIX.$EXTENSION}
        fi
        # coverage build
        if [ "$COVERAGE" == "1" ] ; then
            DB_DESTINATION=${DB_DESTINATION/.$EXTENSION/-coverage.$EXTENSION}
        fi
    fi

    #################
    # rename and tell me
    if [ "$TARGET" == "docs" ] ; then
	echo ""
	echo "renaming    [$RENAME_SOURCE_DOCS]"
	echo "         to [$RENAME_DESTINATION_DOCS]"
	mv $RENAME_SOURCE_DOCS $RENAME_DESTINATION_DOCS
    else
	if [ "$RELEASE" == "1" ] ; then
	    echo ""
	    echo "renaming    [$RENAME_SOURCE_ICOMMANDS]"
	    echo "         to [$RENAME_DESTINATION_ICOMMANDS]"
	    mv $RENAME_SOURCE_ICOMMANDS $RENAME_DESTINATION_ICOMMANDS
	fi
	if [ "$TARGET" == "icat" ] ; then
	    echo ""
	    echo "renaming    [$RENAME_SOURCE_DEV]"
	    echo "         to [$RENAME_DESTINATION_DEV]"
	    mv $RENAME_SOURCE_DEV $RENAME_DESTINATION_DEV
            echo ""
            echo "renaming    [$RENAME_SOURCE_RUNTIME]"
            echo "         to [$RENAME_DESTINATION_RUNTIME]"
            mv $RENAME_SOURCE_RUNTIME $RENAME_DESTINATION_RUNTIME
	fi
        # icat or resource
        echo ""
        echo "renaming    [$RENAME_SOURCE]"
        echo "         to [$RENAME_DESTINATION]"
        mv $RENAME_SOURCE $RENAME_DESTINATION
        # database
        if [ "$BUILDIRODS" == "1" -a "$TARGET" == "icat" ] ; then
            echo ""
            echo "renaming    [$DB_SOURCE]"
            echo "         to [$DB_DESTINATION]"
            mv $DB_SOURCE $DB_DESTINATION
        fi
    fi

    #################
    # list new result set
    echo ""
    echo "Contents of $IRODSPACKAGEDIR:"
    ls -l $IRODSPACKAGEDIR

}

# set up git commit hooks
cd $BUILDDIR
if [ -d ".git/hooks" ] ; then
    cp ./packaging/pre-commit ./.git/hooks/pre-commit
fi

MANDIR=man
# check for clean
if [ "$1" == "clean" ] ; then
    # clean up any build-created files
    echo "${text_green}${text_bold}Clean...${text_reset}"
    echo "Cleaning $SCRIPTNAME residuals..."
    rm -f changelog.gz
    rm -rf $MANDIR
    rm -f manual.pdf
    rm -f irods-manual*.pdf
    rm -f examples/microservices/*.pdf
    rm -f libirods_client.a
    rm -f libirods_server.a

    make clean -C $BUILDDIR --no-print-directory
    set -e
    rm -rf $IRODSPACKAGEDIR
    set +e
    echo "Cleaning EPM residuals..."
    cd $BUILDDIR
    rm -f packaging/irods-dev.list
    rm -f packaging/irods-runtime.list
    rm -f packaging/irods.list
    rm -f packaging/irods-icommands.list
    rm -rf linux-2.*
    rm -rf linux-3.*
    rm -rf macosx-10.*
    rm -f iRODS/server/config/scriptMonPerf.config
    rm -f iRODS/server/config/server.config
    rm -f iRODS/config/irods.config
    rm -f iRODS/lib/core/include/rodsVersion.hpp
    rm -f iRODS/lib/core/include/irods_ms_home.hpp
    rm -f iRODS/lib/core/include/irods_network_home.hpp
    rm -f iRODS/lib/core/include/irods_auth_home.hpp
    rm -f iRODS/lib/core/include/irods_api_home.hpp
    rm -f iRODS/lib/core/include/irods_resources_home.hpp
    rm -f iRODS/server/core/include/irods_database_home.hpp
    rm -f iRODS/lib/core/include/irods_home_directory.hpp
    set -e
    echo "${text_green}${text_bold}Done.${text_reset}"
    # database plugin cleanup
    ./plugins/database/build.sh clean
    rm -f iRODS/config/platform.mk
    rm -f iRODS/config/config.mk
    exit 0
fi

# check for docs
if [ "$1" == "docs" ] ; then
    # building documentation
    echo ""
    echo "${text_green}${text_bold}Building Docs...${text_reset}"
    echo ""

    # get cpu count
    detect_number_of_cpus_and_set_makejcmd

    $MAKEJCMD docs

    # get EPM

    $MAKEJCMD epm

    # prepare list file from template
    cd $BUILDDIR
    LISTFILE="./packaging/irods-docs.list"
    set_tmpfile
    sed -e "s,TEMPLATE_IRODSVERSIONINT,$IRODSVERSIONINT," $LISTFILE.template > $TMPFILE
    mv $TMPFILE $LISTFILE
    sed -e "s,TEMPLATE_IRODSVERSION,$IRODSVERSION,g" $LISTFILE > $TMPFILE
    mv $TMPFILE $LISTFILE

    # package them up
    cd $BUILDDIR
    unamem=`uname -m`
    if [[ "$unamem" == "x86_64" || "$unamem" == "amd64" ]] ; then
	arch="amd64"
    else
	arch="i386"
    fi
    if [ "$DETECTEDOS" == "RedHatCompatible" ] ; then # CentOS and RHEL and Fedora
	echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS RPMs${text_reset}"
	$EPMCMD -f rpm irods-docs $LISTFILE
    elif [ "$DETECTEDOS" == "SuSE" ] ; then # SuSE
	echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS RPMs${text_reset}"
	$EPMCMD -f rpm irods-docs $LISTFILE
    elif [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then  # Ubuntu
	echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DEBs${text_reset}"
	$EPMCMD -a $arch -f deb irods-docs $LISTFILE
    elif [ "$DETECTEDOS" == "Solaris" ] ; then  # Solaris
	echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS PKGs${text_reset}"
	$EPMCMD -f pkg irods-docs $LISTFILE
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then  # MacOSX
	echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DMGs${text_reset}"
	$EPMCMD -f osx irods-docs $LISTFILE
    elif [ "$DETECTEDOS" == "ArchLinux" ] ; then  # ArchLinux
	echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS TGZs${text_reset}"
	$EPMCMD -f portable irods-docs $LISTFILE
    elif [ "$DETECTEDOS" == "Portable" ] ; then  # Portable
	echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS TGZs${text_reset}"
	$EPMCMD -f portable irods-docs $LISTFILE
    else
	echo "${text_red}#######################################################" 1>&2
	echo "ERROR :: Unknown OS, cannot generate packages with EPM" 1>&2
	echo "#######################################################${text_reset}" 1>&2
	exit 1
    fi

    # rename generated packages appropriately
    rename_generated_packages $1

    # done
    echo "${text_green}${text_bold}Done.${text_reset}"
    exit 0
fi


# check for invalid switch combinations
if [[ $1 != "icat" && $1 != "resource" ]] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Invalid serverType [$1]" 1>&2
    echo "      :: Only 'icat' or 'resource' available at this time" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi

if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
    if [ "$(id -u)" != "0" -a "$RUNINPLACE" == "0" ] ; then
        echo "${text_red}#######################################################" 1>&2
        echo "ERROR :: $SCRIPTNAME must be run as root" 1>&2
        echo "      :: because dpkg demands to be run as root" 1>&2
        echo "#######################################################${text_reset}" 1>&2
        exit 1
    fi
fi

################################################################################
# housekeeping - update examples - keep them current
#set_tmpfile
#sed -e s,unix,example,g $BUILDDIR/plugins/resources/unixfilesystem/libunixfilesystem.cpp > $TMPFILE
#. $BUILDDIR/packaging/astyleparams
#if [ "`which astyle`" != "" ] ; then
#    astyle $ASTYLE_PARAMETERS $TMPFILE
#else
#    echo "Skipping formatting --- Artistic Style (astyle) not available"
#fi
#rsync -c $TMPFILE $BUILDDIR/examples/resources/libexamplefilesystem.cpp
#rm -f $TMPFILE

################################################################################
# use error codes to determine dependencies
# does not work on solaris ('which' returns 0, regardless), so check the output as well
set +e

GPLUSPLUS=`which g++`
if [[ "$?" != "0" || `echo $GPLUSPLUS | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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

# needed for boost, of all things...
PYTHONDEV=`find /usr -name Python.h 2> /dev/null`
if [[ "$PYTHONDEV" == "" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT python-dev"
    fi
else
    echo "Detected Python.h [$PYTHONDEV]"
fi

# needed for rpmbuild
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

CURL=`which curl`
if [[ "$?" != "0" || `echo $CURL | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT curl"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT curl"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT curl"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT curl"
    elif [ "$DETECTEDOS" == "MacOSX" ] ; then
        PREFLIGHT="$PREFLIGHT curl"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://curl.haxx.se/download.html"
    fi
else
    CURLVERSION=`curl --version | head -n1 | awk '{print $2}'`
    echo "Detected curl [$CURL] v[$CURLVERSION]"
fi

WGET=`which wget`
if [[ "$?" != "0" || `echo $WGET | awk '{print $1}'` == "no" ]] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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

LIBFUSEDEV=`find /usr/include -name fuse.h 2> /dev/null | $GREPCMD -v linux`
if [ "$LIBFUSEDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT libfuse-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT fuse-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT fuse-devel"
#    elif [ "$DETECTEDOS" == "Solaris" ] ; then
#        No libfuse packages in pkgutil
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://sourceforge.net/projects/fuse/files/fuse-2.X/"
    fi
else
    echo "Detected libfuse library [$LIBFUSEDEV]"
fi

LIBCURLDEV=`find /usr -name curl.h 2> /dev/null`
if [ "$LIBCURLDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT libcurl4-gnutls-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT curl-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT libcurl-devel"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT curl_devel"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://curl.haxx.se/download.html"
    fi
else
    echo "Detected libcurl library [$LIBCURLDEV]"
fi

BZIP2DEV=`find /usr -name bzlib.h 2> /dev/null`
if [ "$BZIP2DEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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

PAMDEV=`find /usr/include -name pam_appl.h 2> /dev/null`
if [ "$PAMDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT libpam0g-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT pam-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT pam-devel"
    # Solaris comes with SUNWhea which provides /usr/include/security/pam_appl.h
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://sourceforge.net/projects/openpam/files/openpam/"
    fi
else
    echo "Detected pam library [$PAMDEV]"
fi

OPENSSLDEV=`find /usr/include/openssl /opt/csw/include/openssl -name sha.h 2> /dev/null`
if [ "$OPENSSLDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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

# needed for lib_mysqludf_preg
MYSQLDEV=`find /usr/include/mysql /opt/csw/include/mysql -name mysql.h 2> /dev/null`
if [ "$MYSQLDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT libmysqlclient-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT mysql-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT libmysqlclient-devel"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT mysql_dev"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://dev.mysql.com/downloads/"
    fi
else
    echo "Detected mysql library [$MYSQLDEV]"
fi

# needed for lib_mysqludf_preg
PCREDEV=`find /usr/include/ /opt/csw/include/ -name pcre.h 2> /dev/null`
if [ "$PCREDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT libpcre3-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT pcre-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT pcre-devel"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT libpcre_dev"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.pcre.org/"
    fi
else
    echo "Detected pcre library [$PCREDEV]"
fi

# needed for libs3
LIBXML2DEV=`find /usr/include/libxml2 /opt/csw/include/libxml2 -name parser.h 2> /dev/null`
if [ "$LIBXML2DEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT libxml2-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT libxml2-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT libxml2-devel"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT libxml2_dev"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://www.xmlsoft.org/downloads.html"
    fi
else
    echo "Detected libxml2 library [$LIBXML2DEV]"
fi

# needed for gsi auth capabilities
KRB5DEV=`find /usr/include /opt/csw/include -name gssapi.h 2> /dev/null`
if [ "$KRB5DEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
        PREFLIGHT="$PREFLIGHT libkrb5-dev"
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then
        PREFLIGHT="$PREFLIGHT krb5-devel"
    elif [ "$DETECTEDOS" == "SuSE" ] ; then
        PREFLIGHT="$PREFLIGHT krb5-devel"
    elif [ "$DETECTEDOS" == "Solaris" ] ; then
        PREFLIGHT="$PREFLIGHT libkrb5_dev"
    else
        PREFLIGHTDOWNLOAD=$'\n'"$PREFLIGHTDOWNLOAD      :: download from: http://web.mit.edu/kerberos/dist/index.html"
    fi
else
    echo "Detected krb5 library [$KRB5DEV]"
fi


# print out prerequisites error
if [ "$PREFLIGHT" != "" ] ; then
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: $SCRIPTNAME requires some software to be installed" 1>&2
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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


# find number of cpus
detect_number_of_cpus_and_set_makejcmd


echo "-----------------------------"
echo "${text_green}${text_bold}Configuring and Building iRODS${text_reset}"
echo "-----------------------------"

# set up own temporary configfile
cd $BUILDDIR/iRODS
TMPCONFIGFILE=/tmp/$USER/irods.config.epm
mkdir -p $(dirname $TMPCONFIGFILE)


# =-=-=-=-=-=-=-
# generate canonical version information for the code from top level VERSION file
cd $BUILDDIR
TEMPLATE_RODS_RELEASE_VERSION=`$GREPCMD "\<IRODSVERSION\>" VERSION | awk -F= '{print $2}'`
TEMPLATE_RODS_RELEASE_DATE=`date +"%b %Y"`
sed -e "s,TEMPLATE_RODS_RELEASE_VERSION,$TEMPLATE_RODS_RELEASE_VERSION," ./iRODS/lib/core/include/rodsVersion.hpp.template > /tmp/rodsVersion.hpp
sed -e "s,TEMPLATE_RODS_RELEASE_DATE,$TEMPLATE_RODS_RELEASE_DATE," /tmp/rodsVersion.hpp > /tmp/rodsVersion.hpp.2
rsync -c /tmp/rodsVersion.hpp.2 ./iRODS/lib/core/include/rodsVersion.hpp
rm -f /tmp/rodsVersion.hpp
rm -f /tmp/rodsVersion.hpp.2

# set up variables for icat configuration
cd $BUILDDIR/iRODS
if [ $1 == "icat" ] ; then
    SERVER_TYPE="ICAT"
    SERVER_TYPE_LOWERCASE="icat"
    # otherwise set up variables for resource configuration
    EPMFILE="../packaging/irods.config.icat.epm"
    cp $EPMFILE $TMPCONFIGFILE
else
    SERVER_TYPE="RESOURCE"
    SERVER_TYPE_LOWERCASE="resource"
    EPMFILE="../packaging/irods.config.resource.epm"
    cp $EPMFILE $TMPCONFIGFILE
fi


if [ "$BUILDIRODS" == "1" ] ; then

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
    # stage a tmp copy of irods.config in order for the utils 
    # script to find it.  otherwise it errors out
    cp ./config/irods.config.template ./config/irods.config

    # =-=-=-=-=-=-=-
    # run configure to create Makefile, config.mk, platform.mk, etc.
    ./scripts/configure
    # overwrite with our values
    cp $TMPCONFIGFILE ./config/irods.config
    # run with our updated irods.config
    ./scripts/configure
    # again to reset IRODS_HOME
    cp $TMPCONFIGFILE ./config/irods.config

    # handle issue with IRODS_HOME being overwritten by the configure script
    if [ "$RUNINPLACE" = "1" ] ; then
        irodsctl_irods_home=`./scripts/find_irods_home.sh runinplace`
    else
        irodsctl_irods_home=`./scripts/find_irods_home.sh`
    fi
    set_tmpfile
    sed -e "\,^IRODS_HOME,s,^.*$,IRODS_HOME=$irodsctl_irods_home," ./irodsctl > $TMPFILE
    rsync -c $TMPFILE ./irodsctl
    rm -f $TMPFILE
    chmod 755 ./irodsctl

    # update build_dir to our absolute path
    set_tmpfile
    sed -e "\,^IRODS_BUILD_DIR=,s,^.*$,IRODS_BUILD_DIR=$BUILDDIR," ./config/config.mk > $TMPFILE
    mv $TMPFILE ./config/config.mk

    # update cpu count to our detected cpu count
    set_tmpfile
    sed -e "\,^CPU_COUNT=,s,^.*$,CPU_COUNT=$CPUCOUNT," ./config/config.mk > $TMPFILE
    mv $TMPFILE ./config/config.mk

    # twiddle coverage flag in platform.mk based on whether this is a coverage (gcov) build
    if [ "$COVERAGE" == "1" ] ; then
        set_tmpfile
        sed -e "s,IRODS_BUILD_COVERAGE=0,IRODS_BUILD_COVERAGE=1," ./config/platform.mk > $TMPFILE
        mv $TMPFILE ./config/platform.mk
    fi

    # twiddle debug flag in platform.mk based on whether this is a release build
    if [ "$RELEASE" == "1" ] ; then
        set_tmpfile
        sed -e "s,IRODS_BUILD_DEBUG=1,IRODS_BUILD_DEBUG=0," ./config/platform.mk > $TMPFILE
        mv $TMPFILE ./config/platform.mk
    fi

    # =-=-=-=-=-=-=-
    # modify the irods_ms_home.hpp file with the proper path to the binary directory
    detected_irods_home=`./scripts/find_irods_home.sh`
    if [ "$RUNINPLACE" = "1" ] ; then
        detected_irods_home=`./scripts/find_irods_home.sh runinplace`
    else
        detected_irods_home=`./scripts/find_irods_home.sh`
    fi
    detected_irods_home=`dirname $detected_irods_home`
    irods_msvc_home="$detected_irods_home/plugins/microservices/"
    set_tmpfile
    sed -e s,IRODSMSVCPATH,$irods_msvc_home, ./lib/core/include/irods_ms_home.hpp.src > $TMPFILE
    rsync -c $TMPFILE ./lib/core/include/irods_ms_home.hpp
    rm -f $TMPFILE
    # =-=-=-=-=-=-=-
    # modify the irods_network_home.hpp file with the proper path to the binary directory
    irods_network_home="$detected_irods_home/plugins/network/"
    set_tmpfile
    sed -e s,IRODSNETWORKPATH,$irods_network_home, ./lib/core/include/irods_network_home.hpp.src > $TMPFILE
    rsync -c $TMPFILE ./lib/core/include/irods_network_home.hpp
    rm -f $TMPFILE
    # =-=-=-=-=-=-=-
    # modify the irods_auth_home.hpp file with the proper path to the binary directory
    irods_auth_home="$detected_irods_home/plugins/auth/"
    set_tmpfile
    sed -e s,IRODSAUTHPATH,$irods_auth_home, ./lib/core/include/irods_auth_home.hpp.src > $TMPFILE
    rsync -c $TMPFILE ./lib/core/include/irods_auth_home.hpp
    rm -f $TMPFILE
    # =-=-=-=-=-=-=-
    # modify the irods_resources_home.hpp file with the proper path to the binary directory
    irods_resources_home="$detected_irods_home/plugins/resources/"
    set_tmpfile
    sed -e s,IRODSRESOURCESPATH,$irods_resources_home, ./lib/core/include/irods_resources_home.hpp.src > $TMPFILE
    rsync -c $TMPFILE ./lib/core/include/irods_resources_home.hpp
    rm -f $TMPFILE
    # =-=-=-=-=-=-=-
    # modify the irods_database_home.hpp file with the proper path to the binary directory
    irods_database_home="$detected_irods_home/plugins/database/"
    set_tmpfile
    sed -e s,IRODSDATABASEPATH,$irods_database_home, ./server/core/include/irods_database_home.hpp.src > $TMPFILE
    rsync -c $TMPFILE ./server/core/include/irods_database_home.hpp
    rm -f $TMPFILE
    # =-=-=-=-=-=-=-
    # modify the irods_api_home.hpp file with the proper path to the binary directory
    irods_api_home="$detected_irods_home/plugins/api/"
    set_tmpfile
    sed -e s,IRODSAPIPATH,$irods_api_home, ./lib/core/include/irods_api_home.hpp.src > $TMPFILE
    rsync -c $TMPFILE ./lib/core/include/irods_api_home.hpp
    rm -f $TMPFILE
    # =-=-=-=-=-=-=-
    # modify the irods_home_directory.hpp file with the proper path to the home directory
    irods_home_directory="$detected_irods_home/"
    set_tmpfile
    sed -e s,IRODSHOMEDIRECTORY,$irods_home_directory, ./lib/core/include/irods_home_directory.hpp.src > $TMPFILE
    rsync -c $TMPFILE ./lib/core/include/irods_home_directory.hpp
    rm -f $TMPFILE

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
    if [ "$SERVER_TYPE" == "ICAT" ] ; then
        # build icat package
        $MAKEJCMD -C $BUILDDIR icat-package
        # build designated database plugin
        $BUILDDIR/plugins/database/build.sh $2
    elif [ "$SERVER_TYPE" == "RESOURCE" ] ; then
        # build resource package
        $MAKEJCMD -C $BUILDDIR resource-package
    fi
    if [ "$?" != "0" ] ; then
        exit 1
    fi

    # =-=-=-=-=-=-=-
    # exit early for run-in-place option
    if [ "$RUNINPLACE" == "1" ] ; then
        echo "YUNOPACKAGE?"
        exit 0
    fi

    # =-=-=-=-=-=-=-
    # populate IRODSVERSIONINT and IRODSVERSION in all EPM list files

    # irods main package
    cd $BUILDDIR
    set_tmpfile
    sed -e "s,TEMPLATE_IRODSVERSIONINT,$IRODSVERSIONINT," ./packaging/irods.list.template > $TMPFILE
    mv $TMPFILE ./packaging/irods.list
    sed -e "s,TEMPLATE_IRODSVERSION,$IRODSVERSION," ./packaging/irods.list > $TMPFILE
    mv $TMPFILE ./packaging/irods.list
    # irods-dev package
    sed -e "s,TEMPLATE_IRODSVERSIONINT,$IRODSVERSIONINT," ./packaging/irods-dev.list.template > $TMPFILE
    mv $TMPFILE ./packaging/irods-dev.list
    sed -e "s,TEMPLATE_IRODSVERSION,$IRODSVERSION," ./packaging/irods-dev.list > $TMPFILE
    mv $TMPFILE ./packaging/irods-dev.list
    # irods-runtime package
    sed -e "s,TEMPLATE_IRODSVERSIONINT,$IRODSVERSIONINT," ./packaging/irods-runtime.list.template > $TMPFILE
    mv $TMPFILE ./packaging/irods-runtime.list
    sed -e "s,TEMPLATE_IRODSVERSION,$IRODSVERSION," ./packaging/irods-runtime.list > $TMPFILE
    mv $TMPFILE ./packaging/irods-runtime.list
    # irods-icommands package
    sed -e "s,TEMPLATE_IRODSVERSIONINT,$IRODSVERSIONINT," ./packaging/irods-icommands.list.template > $TMPFILE
    mv $TMPFILE ./packaging/irods-icommands.list
    sed -e "s,TEMPLATE_IRODSVERSION,$IRODSVERSION," ./packaging/irods-icommands.list > $TMPFILE
    mv $TMPFILE ./packaging/irods-icommands.list


#    set +e
#    # generate microservice developers tutorial in pdf format
#    echo "${text_green}${text_bold}Building iRODS Microservice Developers Tutorial${text_reset}"
#    cd $BUILDDIR/examples/microservices
#    rst2pdf microservice_tutorial.rst -o microservice_tutorial.pdf
#    if [ "$?" != "0" ] ; then
#        echo "${text_red}#######################################################" 1>&2
#        echo "ERROR :: Failed generating microservice_tutorial.pdf" 1>&2
#        echo "#######################################################${text_reset}" 1>&2
#        exit 1
#    fi
#    set -e

    # generate tgz file for inclusion in coverage package
    if [ "$COVERAGE" == "1" ] ; then
        set +e
        GCOVFILELIST="gcovfilelist.txt"
        GCOVFILENAME="gcovfiles.tgz"
        cd $BUILDDIR
        find ./plugins ./iRODS -name "*.h" -o -name "*.c" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.gcno" > $GCOVFILELIST
        tar czf $GCOVFILENAME -T $GCOVFILELIST
        ls -al $GCOVFILELIST
        ls -al $GCOVFILENAME
        set -e
    fi

    # generate development package archive file
    if [ "$1" == "icat" ] ; then
        echo "${text_green}${text_bold}Building development package archive file...${text_reset}"
        cd $BUILDDIR
        ./packaging/make_irods_dev_archive.sh
    fi

fi # if $BUILDIRODS


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
    IRODSMANVERSION=`$GREPCMD "^%version" ./packaging/irods.list | awk '{print $2}'`
    ICMDDIR="iRODS/clients/icommands/bin"
    ICMDS=(
    iadmin        
    ibun          
    icd           
    ichksum       
    ichmod        
    icp           
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
        help2man -h -h -N -n "an iRODS iCommand" --version-string="iRODS-$IRODSMANVERSION" $ICMDDIR/$ICMD > $MANDIR/$ICMD.1
    done
    for manfile in `ls $MANDIR`
    do
        gzip -9 $MANDIR/$manfile
    done
fi

if [ "$COVERAGE" == "1" ] ; then
    # sets EPM to not strip binaries of debugging information
    EPMOPTS="-g"
    # sets listfile coverage options
    EPMOPTS="$EPMOPTS COVERAGE=true"
elif [ "$RELEASE" == "1" ] ; then
    EPMOPTS=""
else
    EPMOPTS="-g"
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
    $EPMCMD $EPMOPTS -f rpm irods-$SERVER_TYPE_LOWERCASE $epmvar=true $epmosversion=true ./packaging/irods.list
    if [ "$SERVER_TYPE" == "ICAT" ] ; then
        $EPMCMD $EPMOPTS -f rpm irods-dev $epmvar=true $epmosversion=true ./packaging/irods-dev.list
        $EPMCMD $EPMOPTS -f rpm irods-runtime $epmvar=true $epmosversion=true ./packaging/irods-runtime.list
    fi
    if [ "$RELEASE" == "1" ] ; then
        $EPMCMD $EPMOPTS -f rpm irods-icommands $epmvar=true $epmosversion=true ./packaging/irods-icommands.list
    fi
elif [ "$DETECTEDOS" == "SuSE" ] ; then # SuSE
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS RPMs${text_reset}"
    epmvar="SUSERPM$SERVER_TYPE"
    $EPMCMD $EPMOPTS -f rpm irods-$SERVER_TYPE_LOWERCASE $epmvar=true ./packaging/irods.list
    if [ "$SERVER_TYPE" == "ICAT" ] ; then
        $EPMCMD $EPMOPTS -f rpm irods-dev $epmvar=true ./packaging/irods-dev.list
        $EPMCMD $EPMOPTS -f rpm irods-runtime $epmvar=true ./packaging/irods-runtime.list
    fi
    if [ "$RELEASE" == "1" ] ; then
        $EPMCMD $EPMOPTS -f rpm irods-icommands $epmvar=true ./packaging/irods-icommands.list
    fi
elif [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then  # Ubuntu
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DEBs${text_reset}"
    epmvar="DEB$SERVER_TYPE"
    $EPMCMD $EPMOPTS -a $arch -f deb irods-$SERVER_TYPE_LOWERCASE $epmvar=true ./packaging/irods.list
    if [ "$SERVER_TYPE" == "ICAT" ] ; then
        $EPMCMD $EPMOPTS -a $arch -f deb irods-dev $epmvar=true ./packaging/irods-dev.list
        $EPMCMD $EPMOPTS -a $arch -f deb irods-runtime $epmvar=true ./packaging/irods-runtime.list
    fi
    if [ "$RELEASE" == "1" ] ; then
        $EPMCMD $EPMOPTS -a $arch -f deb irods-icommands $epmvar=true ./packaging/irods-icommands.list
    fi
elif [ "$DETECTEDOS" == "Solaris" ] ; then  # Solaris
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS PKGs${text_reset}"
    epmvar="PKG$SERVER_TYPE"
    $EPMCMD $EPMOPTS -f pkg irods-$SERVER_TYPE_LOWERCASE $epmvar=true ./packaging/irods.list
    if [ "$SERVER_TYPE" == "ICAT" ] ; then
        $EPMCMD $EPMOPTS -f pkg irods-dev $epmvar=true ./packaging/irods-dev.list
        $EPMCMD $EPMOPTS -f pkg irods-runtime $epmvar=true ./packaging/irods-runtime.list
    fi
    if [ "$RELEASE" == "1" ] ; then
        $EPMCMD $EPMOPTS -f pkg irods-icommands $epmvar=true ./packaging/irods-icommands.list
    fi
elif [ "$DETECTEDOS" == "MacOSX" ] ; then  # MacOSX
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DMGs${text_reset}"
    epmvar="OSX$SERVER_TYPE"
    $EPMCMD $EPMOPTS -f osx irods-$SERVER_TYPE_LOWERCASE $epmvar=true ./packaging/irods.list
    if [ "$SERVER_TYPE" == "ICAT" ] ; then
        $EPMCMD $EPMOPTS -f osx irods-dev $epmvar=true ./packaging/irods-dev.list
        $EPMCMD $EPMOPTS -f osx irods-runtime $epmvar=true ./packaging/irods-runtime.list
    fi
    if [ "$RELEASE" == "1" ] ; then
        $EPMCMD $EPMOPTS -f osx irods-icommands $epmvar=true ./packaging/irods-icommands.list
    fi
elif [ "$DETECTEDOS" == "ArchLinux" ] ; then  # ArchLinux
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS TGZs${text_reset}"
    epmvar="ARCH$SERVERTYPE"
    if [ "$SERVER_TYPE" == "ICAT" ] ; then
        ICAT=true $EPMCMD $EPMOPTS -f portable irods-$SERVER_TYPE_LOWERCASE $epmvar=true ./packaging/irods.list
        $EPMCMD $EPMOPTS -f portable irods-dev $epmvar=true ./packaging/irods-dev.list
        $EPMCMD $EPMOPTS -f portable irods-runtime $epmvar=true ./packaging/irods-runtime.list
    else
        $EPMCMD $EPMOPTS -f portable irods-$SERVER_TYPE_LOWERCASE $epmvar=true ./packaging/irods.list
    fi
    if [ "$RELEASE" == "1" ] ; then
        $EPMCMD $EPMOPTS -f portable irods-icommands $epmvar=true ./packaging/irods-icommands.list
    fi
elif [ "$DETECTEDOS" == "Portable" ] ; then  # Portable
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS TGZs${text_reset}"
    epmvar="PORTABLE$SERVER_TYPE"
    $EPMCMD $EPMOPTS -f portable irods-$SERVER_TYPE_LOWERCASE $epmvar=true ./packaging/irods.list
    if [ "$SERVER_TYPE" == "ICAT" ] ; then
        ICAT=true $EPMCMD $EPMOPTS -f portable irods-dev $epmvar=true ./packaging/irods-dev.list
        ICAT=true $EPMCMD $EPMOPTS -f portable irods-runtime $epmvar=true ./packaging/irods-runtime.list
    fi
    if [ "$RELEASE" == "1" ] ; then
        $EPMCMD $EPMOPTS -f portable irods-icommands $epmvar=true ./packaging/irods-icommands.list
    fi
else
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Unknown OS, cannot generate packages with EPM" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi


# rename generated packages appropriately
rename_generated_packages $1

# clean up coverage build
if [ "$COVERAGE" == "1" ] ; then
    # copy important bits back up
    echo "${text_green}${text_bold}Copying generated packages back to original working directory...${text_reset}"
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
echo "| RENCI iRODS Build Script           |"
echo "|                                    |"
printf "|   Completed in %02dm%02ds              |\n" "$((TOTALTIME/60))" "$((TOTALTIME%60))"
echo "+------------------------------------+"
echo "${text_reset}"
