#!/bin/bash

# =-=-=-=-=-=-=-
# handle the case of build clean
if [ "$1" == "clean" ] ; then
    rm -f server/icat/src/icatCoreTables.sql
    rm -f server/icat/src/icatSysTables.sql
    make clean
    exit 0
fi

# =-=-=-=-=-=-=-
# set up some variables
DB_TYPE=$1
TMPCONFIGFILE="/tmp/$USER/irods.config.epm"
mkdir -p $(dirname $TMPCONFIGFILE)
    
EPMFILE="../../packaging/irods.config.icat.epm"
cp $EPMFILE $TMPCONFIGFILE

# prepare list file from template
source ./${DB_TYPE}/VERSION
echo "Detected Plugin Version to Build [$PLUGINVERSION]"
echo "Detected Plugin Version Integer  [$PLUGINVERSIONINT]"
LISTFILE="./packaging/irods_database_plugin_${DB_TYPE}.list"
TMPFILE="/tmp/irods_db_plugin.list"
sed -e "s,TEMPLATE_PLUGINVERSIONINT,$PLUGINVERSIONINT," $LISTFILE.template > $TMPFILE
mv $TMPFILE $LISTFILE
sed -e "s,TEMPLATE_PLUGINVERSION,$PLUGINVERSION," $LISTFILE > $TMPFILE
mv $TMPFILE $LISTFILE

# =-=-=-=-=-=-=-
# determine the OS Flavor
DETECTEDOS=`./packaging/find_os.sh`
if [ "$PORTABLE" == "1" ] ; then
  DETECTEDOS="Portable"
fi
echo "Detected OS [$DETECTEDOS]"

# =-=-=-=-=-=-=-
# determine the OS Version
DETECTEDOSVERSION=`./packaging/find_os_version.sh`
echo "Detected OS Version [$DETECTEDOSVERSION]"

# =-=-=-=-=-=-=-
# handle determination of DB location
FIND_DB="./packaging/find_${DB_TYPE}_bin.sh"
DB_BIN=`${FIND_DB}`

if [ "$DB_BIN" == "FAIL" ] ; then
    if [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then
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
    echo "Detected DB [$DB_BIN]"
fi

# =-=-=-=-=-=-=-
# handle determination of ODBC location
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

# =-=-=-=-=-=-=-
# bake SQL files for different database types
# NOTE:: icatSysInserts.sql is handled by the packager as we rely on the default zone name
serverSqlDir="./src"
convertScript="$serverSqlDir/convertSql.pl"

echo "Converting SQL: [$convertScript] [$DB_TYPE] [$serverSqlDir]"
`perl $convertScript $DB_TYPE $serverSqlDir &> /dev/null`
if [ "$?" -ne "0" ] ; then
    echo "Failed to convert SQL forms" 1>&2
    exit 1
fi

# =-=-=-=-=-=-=-
# insert DB into list file
# need to do a dirname here, as the irods.config is expected to have a path
# which will be appended with a /bin
IRODS_DB_PATH=`./packaging/find_${DB_TYPE}_bin.sh`
if [ "$IRODS_DB_PATH" == "FAIL" ] ; then
    exit 1
fi
IRODS_DB_PATH=`dirname $IRODS_DB_PATH`
IRODS_DB_PATH="$IRODS_DB_PATH/"

echo "Detected ${DB_TYPE} path [$IRODS_DB_PATH]"
sed -e s,IRODS_DB_PATH,$IRODS_DB_PATH, $EPMFILE > $TMPCONFIGFILE

SETUP_FILE="./packaging/setup_database.sh"
sed -e s,TEMPLATE_DATABASE_TYPE,$DB_TYPE, "$SETUP_FILE.template" > "/tmp/$USER/setup_database.temp"
mv "/tmp/$USER/setup_database.temp" $SETUP_FILE

# =-=-=-=-=-=-=-
# build the particular flavor of DB plugin
make ${DB_TYPE}

# =-=-=-=-=-=-=-
# package the plugin and associated files

if [ "$COVERAGE" == "1" ] ; then
    # sets EPM to not strip binaries of debugging information
    EPMOPTS="-g"
    # sets listfile coverage options
    EPMOPTS="$EPMOPTS COVERAGE=true"
else
    EPMOPTS=""
fi

# =-=-=-=-=-=-=-
# determine appropriate architecture
unamem=`uname -m`
if [[ "$unamem" == "x86_64" || "$unamem" == "amd64" ]] ; then
    arch="amd64"
else
    arch="i386"
fi

EPMCMD="../../external/epm/epm"
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
    $EPMCMD $EPMOPTS -f rpm irods-database-plugin-${DB_TYPE}  $epmvar=true $epmosversion=true $LISTFILE
elif [ "$DETECTEDOS" == "SuSE" ] ; then # SuSE
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS RPMs${text_reset}"
    epmvar="SUSERPM$SERVER_TYPE"
    $EPMCMD $EPMOPTS -f rpm irods-database-plugin-${DB_TYPE}  $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "Ubuntu" -o "$DETECTEDOS" == "Debian" ] ; then  # Ubuntu
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DEBs${text_reset}"
    epmvar="DEB$SERVER_TYPE"
    FOO="$EPMCMD $EPMOPTS -a $arch -f deb irods-database-plugin-${DB_TYPE}  $epmvar=true $LISTFILE"
    echo "EPM COMMAND: [$FOO]"
    $EPMCMD $EPMOPTS -a $arch -f deb irods-database-plugin-${DB_TYPE}  $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "Solaris" ] ; then  # Solaris
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS PKGs${text_reset}"
    epmvar="PKG$SERVER_TYPE"
    $EPMCMD $EPMOPTS -f pkg irods-database-plugin-${DB_TYPE}  $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "MacOSX" ] ; then  # MacOSX
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS DMGs${text_reset}"
    epmvar="OSX$SERVER_TYPE"
    $EPMCMD $EPMOPTS -f osx irods-database-plugin-${DB_TYPE}  $epmvar=true $LISTFILE
elif [ "$DETECTEDOS" == "Portable" ] ; then  # Portable
    echo "${text_green}${text_bold}Running EPM :: Generating $DETECTEDOS TGZs${text_reset}"
    epmvar="PORTABLE$SERVER_TYPE"
    $EPMCMD $EPMOPTS -f portable irods-database-plugin-${DB_TYPE}  $epmvar=true $LISTFILE
else
    echo "${text_red}#######################################################" 1>&2
    echo "ERROR :: Unknown OS, cannot generate packages with EPM" 1>&2
    echo "#######################################################${text_reset}" 1>&2
    exit 1
fi




