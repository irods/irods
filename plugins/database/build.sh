#!/bin/bash -e

# =-=-=-=-=-=-=-
# detect environment
SCRIPTNAME=`basename $0`
SCRIPTPATH=$( cd $(dirname $0) ; pwd -P )
FULLPATHSCRIPTNAME=$SCRIPTPATH/$SCRIPTNAME
BUILDDIR=$( cd $SCRIPTPATH/../../ ; pwd -P )
cd $SCRIPTPATH

source "$BUILDDIR/packaging/irods_externals_locations.mk"

# =-=-=-=-=-=-=-
# check input
USAGE="
Usage:
  $SCRIPTNAME clean
  $SCRIPTNAME postgres
  $SCRIPTNAME mysql
  $SCRIPTNAME oracle

Options:
-c, --coverage        Build with coverage support (gcov)
-h, --help            Show this help
-r, --release         Build a release package (no debugging information, optimized)
    --run-in-place    Build for in-place execution (not recommended)
-v, --verbose         Show the actual compilation commands executed
"

# translate long options to short
for arg
do
    delim=""
    case "$arg" in
        --coverage) args="${args}-c ";;
        --help) args="${args}-h ";;
        --release) args="${args}-r ";;
        --verbose) args="${args}-v ";;
        --run-in-place) args="${args}-z ";;
        # pass through anything else
        *) [[ "${arg:0:1}" == "-" ]] || delim="\""
        args="${args}${delim}${arg}${delim} ";;
    esac
done
# reset the translated args
eval set -- $args
# now we can process with getopts
while getopts ":chrvz" opt; do
    case $opt in
        c)
        COVERAGE="1"
        echo "-c, --coverage detected -- Building iRODS with coverage support (gcov)"
        ;;
        h)
        echo "$USAGE"
        ;;
        r)
        RELEASE="1"
        echo "-r, --release detected -- Building a RELEASE package"
        ;;
        z)
        RUNINPLACE="1"
        echo "--run-in-place detected -- Building for in-place execution"
        ;;
        v)
        VERBOSE="1"
        echo "-v, --verbose detected -- Showing compilation commands"
        ;;
        \?)
        echo "Invalid option: -$OPTARG" >&2
        ;;
    esac
done
echo ""

# remove options from $@
shift $((OPTIND-1))

if [ $# -eq 0 -o $# -gt 1 -o "$1" == "-h" -o "$1" == "--help" -o "$1" == "help" ] ; then
    echo "$USAGE"
    exit 1
fi

# =-=-=-=-=-=-=-
# handle the case of build clean
if [ "$1" == "clean" ] ; then
    rm -f $SCRIPTPATH/src/icatSysTables.sql
    rm -f $SCRIPTPATH/packaging/irods_database_plugin_*.list
    rm -f $SCRIPTPATH/packaging/setup_irods_database.sh
    make clean
    exit 0
fi

# =-=-=-=-=-=-=-
# check database type
DB_TYPE=$1
if [ "$DB_TYPE" != "postgres" -a "$DB_TYPE" != "mysql" -a "$DB_TYPE" != "oracle" ] ; then
    echo ""
    echo "ERROR :: Invalid Database Type [$DB_TYPE]"
    echo "$USAGE"
    exit 1
fi

# =-=-=-=-=-=-=-
# tmpfile function
function set_tmpfile {
  TMPFILE=/tmp/$USER/$(date "+%Y%m%d-%H%M%S.%N.irods.tmp")
}

# =-=-=-=-=-=-=-
# set up some variables
mkdir -p /tmp/$USER

# prepare list file from template
source $SCRIPTPATH/${DB_TYPE}/VERSION
echo "Detected Plugin Version to Build [$PLUGINVERSION]"
echo "Detected Plugin Version Integer  [$PLUGINVERSIONINT]"
LISTFILE="$SCRIPTPATH/packaging/irods_database_plugin_${DB_TYPE}.list"
set_tmpfile
sed -e "s,TEMPLATE_PLUGINVERSIONINT,$PLUGINVERSIONINT," $LISTFILE.template > $TMPFILE; mv $TMPFILE $LISTFILE
sed -e "s,TEMPLATE_PLUGINVERSION,$PLUGINVERSION," $LISTFILE > $TMPFILE; mv $TMPFILE $LISTFILE

# =-=-=-=-=-=-=-
# determine the OS Flavor
DETECTEDOS=`$BUILDDIR/packaging/find_os.sh`
if [ "$PORTABLE" == "1" ] ; then
  DETECTEDOS="Portable"
fi
echo "Detected OS [$DETECTEDOS]"

# =-=-=-=-=-=-=-
# determine the OS Version
DETECTEDOSVERSION=`$BUILDDIR/packaging/find_os_version.sh`
echo "Detected OS Version [$DETECTEDOSVERSION]"

# need odbc-dev package
set +e
UNIXODBCDEV=`find /opt/csw/include/ /usr/include /usr/local -name sql.h 2> /dev/null`
if [ "$UNIXODBCDEV" == "" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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
set -e

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

# =-=-=-=-=-=-=-
# bake SQL files for different database types
# NOTE:: icatSysInserts.sql is handled by the packager as we rely on the default zone name
serverSqlDir="$SCRIPTPATH/src"
convertScript="$serverSqlDir/convertSql.pl"

echo "Converting SQL: [$convertScript] [$DB_TYPE] [$serverSqlDir]"
`perl $convertScript $DB_TYPE $serverSqlDir &> /dev/null`
if [ "$?" -ne "0" ] ; then
    echo "Failed to convert SQL forms" 1>&2
    exit 1
fi


SETUP_FILE="$SCRIPTPATH/packaging/setup_irods_database.sh"
set_tmpfile
sed -e s,TEMPLATE_DATABASE_TYPE,$DB_TYPE, "$SETUP_FILE.template" > $TMPFILE; mv $TMPFILE $SETUP_FILE

# =-=-=-=-=-=-=-
# setup default port
defaultport="NOTDETECTED"
if [ "$DB_TYPE" == "postgres" ] ;  then defaultport="5432"; fi
if [ "$DB_TYPE" == "mysql" ] ;     then defaultport="3306"; fi
if [ "$DB_TYPE" == "oracle" ] ;    then defaultport="1521"; fi
set_tmpfile
sed -e s,TEMPLATE_DEFAULT_DATABASEPORT,$defaultport, $SETUP_FILE > $TMPFILE; mv $TMPFILE $SETUP_FILE

# =-=-=-=-=-=-=-
# set script as executable
chmod +x $SETUP_FILE

# =-=-=-=-=-=-=-
# build the particular flavor of DB plugin
if [ "$VERBOSE" == "1" ] ; then
    VERBOSITYOPTION="VERBOSE=1"
else
    VERBOSITYOPTION="--no-print-directory"
fi
cd $SCRIPTPATH
make ${VERBOSITYOPTION} ${DB_TYPE}

# =-=-=-=-=-=-=-
# package the plugin and associated files
if [ "$RUNINPLACE" == "1" ] ; then
    # work is done - exit early when building for run-in-place
    exit 0
fi

if [ "$COVERAGE" == "1" ] ; then
    # sets EPM to not strip binaries of debugging information
    EPMOPTS="-g"
    # sets listfile coverage options
    EPMOPTS="$EPMOPTS COVERAGE=true"
else
    EPMOPTS="-g"
fi

# =-=-=-=-=-=-=-
# determine appropriate architecture
unamem=`uname -m`
if [[ "$unamem" == "ppc64le" ]] ; then
    # need to investigate reason behind reversal -- 'le' to 'el'
    arch="ppc64el"
elif [[ "$unamem" == "x86_64" || "$unamem" == "amd64" ]] ; then
    arch="amd64"
else
    arch="i386"
fi

cd $SCRIPTPATH

############################################################
# new build hijinks
if [ "$RUNINPLACE" == "1" ] ; then
    if [ "$IRODS_EXTERNALS_ROOT" == "" ]; then
        if [ -e $IRODS_EXTERNALS_PACKAGE_ROOT ]; then
            echo "Setting IRODS_EXTERNALS_ROOT to installed packages: $IRODS_EXTERNALS_PACKAGE_ROOT"
            IRODS_EXTERNALS_ROOT=$IRODS_EXTERNALS_PACKAGE_ROOT
        else
            echo "irods-externals package not detected, IRODS_EXTERNALS_ROOT must point to the fully-qualified path to the location of the externals"
        fi
    fi
else
    IRODS_EXTERNALS_ROOT="/opt/irods-externals"
fi

EPMCMD=$IRODS_EXTERNALS_ROOT/$BUILD_SUBDIRECTORY_EPM/bin/epm
if [ "$DETECTEDOS" == "RedHatCompatible" ] ; then # CentOS and RHEL and Fedora
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS RPMs${text_reset}"
    epmvar="REDHATRPM"
    ostype=`awk '{print $1}' /etc/redhat-release`
    osversion=`awk '{print $3}' /etc/redhat-release`
    if [ "$ostype" == "CentOS" -a "$osversion" \> "6" ]; then
        epmosversion="CENTOS6"
    else
        epmosversion="NOTCENTOS6"
    fi
    $EPMCMD $EPMOPTS -f rpm irods-database-plugin-${DB_TYPE} $epmvar=true $epmosversion=true $LISTFILE
    if [ "$epmosversion" == "CENTOS6" -a "$DB_TYPE" == "postgres" ] ; then
        # also build a postgres93 version of the list file and packaging
        NINETHREELISTFILE=${LISTFILE/postgres.list/postgres93.list}
        set_tmpfile
        sed -e 's/postgresql-odbc/postgresql93-odbc/' $LISTFILE > $TMPFILE
        sed -e 's/postgresql$/postgresql93/' $TMPFILE > $NINETHREELISTFILE
        $EPMCMD $EPMOPTS -f rpm irods-database-plugin-${DB_TYPE}93 $epmvar=true $epmosversion=true $NINETHREELISTFILE
    fi
elif [ "$DETECTEDOS" == "SuSE" ] ; then # SuSE
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS RPMs${text_reset}"
    epmvar="SUSERPM"
    $EPMCMD $EPMOPTS -f rpm irods-database-plugin-${DB_TYPE} $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then  # Ubuntu
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DEBs${text_reset}"
    epmvar="DEB"
    $EPMCMD $EPMOPTS -a $arch -f deb irods-database-plugin-${DB_TYPE} $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "Solaris" ] ; then  # Solaris
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS PKGs${text_reset}"
    epmvar="PKG"
    $EPMCMD $EPMOPTS -f pkg irods-database-plugin-${DB_TYPE} $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "MacOSX" ] ; then  # MacOSX
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DMGs${text_reset}"
    epmvar="OSX"
    $EPMCMD $EPMOPTS -f osx irods-database-plugin-${DB_TYPE} $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "ArchLinux" ] ; then  # ArchLinux
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS TGZs${text_reset}"
    epmvar="ARCH"
    $EPMCMD $EPMOPTS -f portable irods-database-plugin-${DB_TYPE} $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "Portable" ] ; then  # Portable
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS TGZs${text_reset}"
    epmvar="PORTABLE"
    $EPMCMD $EPMOPTS -f portable irods-database-plugin-${DB_TYPE} $epmvar=true $LISTFILE
else
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Unknown OS, cannot generate packages with EPM" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi
