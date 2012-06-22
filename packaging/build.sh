#!/bin/bash

set -e

SCRIPTNAME=`basename $0`
COVERAGE="0"
RELEASE="0"
BUILDEIRODS="1"
COVERAGEBUILDDIR="/var/lib/e-irods"

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

# boilerplate
echo ""
echo "+------------------------------------+"
echo "| RENCI E-iRODS Build Script         |"
echo "+------------------------------------+"
date
echo ""

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
    echo "#######################################################" 1>&2
    echo "ERROR :: Incompatible options:" 1>&2
    echo "      :: -s   skip compilation" 1>&2
    echo "      :: -r   build for release" 1>&2
    echo "#######################################################" 1>&2
    exit 1
fi
if [ "$BUILDEIRODS" == "0" -a "$COVERAGE" == "1" ] ; then
    echo "#######################################################" 1>&2
    echo "ERROR :: Incompatible options:" 1>&2
    echo "      :: -s   skip compilation" 1>&2
    echo "      :: -c   coverage support" 1>&2
    echo "#######################################################" 1>&2
    exit 1
fi
if [ "$COVERAGE" == "1" -a "$RELEASE" == "1" ] ; then
    echo "#######################################################" 1>&2
    echo "ERROR :: Incompatible options:" 1>&2
    echo "      :: -c   coverage support" 1>&2
    echo "      :: -r   build for release" 1>&2
    echo "#######################################################" 1>&2
    exit 1
fi

if [ "$COVERAGE" == "1" ] ; then
    if [ -d "$COVERAGEBUILDDIR" ] ; then
        echo "#######################################################" 1>&2
        echo "ERROR :: $COVERAGEBUILDDIR already exists" 1>&2
        echo "      :: Cannot build in place with coverage enabled" 1>&2
        echo "#######################################################" 1>&2
        exit 1
    fi
    if [ "$(id -u)" != "0" ] ; then
        echo "#######################################################" 1>&2
        echo "ERROR :: $SCRIPTNAME must be run as root" 1>&2
        echo "      :: when building in place (coverage enabled)" 1>&2
        echo "#######################################################" 1>&2
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

MANDIR=man
# check for clean
if [ "$1" == "clean" ] ; then
    # clean up any build-created files
    echo "Clean..."
    echo "Cleaning $SCRIPTNAME residuals..."
    rm -f changelog.gz
    rm -rf $MANDIR
    rm -f manual.pdf
    set +e
    echo "Cleaning EPM residuals..."
    rm -rf linux-2.*
    rm -rf linux-3.*
    rm -rf macosx-10.*
    cd epm
    make clean > /dev/null 2>&1
    make distclean > /dev/null 2>&1
    echo "Cleaning iRODS residuals..."
    cd ../iRODS
    make clean > /dev/null 2>&1
    rm -rf doc/html
    rm -f server/config/reConfigs/raja1.re
    rm -f server/config/scriptMonPerf.config
    rm -f server/icat/src/icatCoreTables.sql
    rm -f server/icat/src/icatSysTables.sql
    set -e
    echo "Done."
    exit 0
fi


# get into the correct directory 
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DETECTEDDIR/../
GITDIR=`pwd`
BUILDDIR=$GITDIR  # we'll manipulate this later, depending on the coverage flag
cd $BUILDDIR/iRODS
echo "Detected Packaging Directory [$DETECTEDDIR]"
echo "Build Directory set to [$BUILDDIR]"
# detect operating system
DETECTEDOS=`../packaging/find_os.sh`
echo "Detected OS [$DETECTEDOS]"


if [ $1 != "icat" -a $1 != "resource" ] ; then
    echo "#######################################################" 1>&2
    echo "ERROR :: Invalid serverType [$1]" 1>&2
    echo "      :: Only 'icat' or 'resource' available at this time" 1>&2
    echo "#######################################################" 1>&2
    exit 1
fi

if [ "$1" == "icat" ] ; then
    #  if [ "$2" != "postgres" -a "$2" != "mysql" ]
    if [ "$2" != "postgres" ] ; then
        echo "#######################################################" 1>&2
        echo "ERROR :: Invalid iCAT databaseType [$2]" 1>&2
        echo "      :: Only 'postgres' available at this time" 1>&2
        echo "#######################################################" 1>&2
        exit 1
    fi
fi

if [ "$DETECTEDOS" == "Ubuntu" ] ; then # Ubuntu
    if [ "$(id -u)" != "0" ] ; then
        echo "#######################################################" 1>&2
        echo "ERROR :: $SCRIPTNAME must be run as root" 1>&2
        echo "      :: because dpkg demands to be run as root" 1>&2
        echo "#######################################################" 1>&2
        exit 1
    fi
fi

# use error codes to determine dependencies
set +e

RST2PDF=`which rst2pdf`
if [ "$?" -ne "0" ] ; then
    echo "#######################################################" 1>&2
    echo "ERROR :: $SCRIPTNAME requires rst2pdf to be installed" 1>&2
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then # Ubuntu
        echo "      :: try: apt-get install rst2pdf" 1>&2
    else
        echo "      :: try: easy_install rst2pdf" 1>&2
    fi
    echo "#######################################################" 1>&2
    exit 1
fi

ROMAN=`python -c "import roman"`
if [ "$?" -ne "0" ] ; then
    echo "#######################################################" 1>&2
    echo "ERROR :: rst2pdf requires python module 'roman' to be installed" 1>&2
    echo "      :: try: easy_install roman" 1>&2
    echo "#######################################################" 1>&2
    exit 1
fi

DOXYGEN=`which doxygen`
if [ "$?" -ne "0" ] ; then
    echo "#######################################################" 1>&2
    echo "ERROR :: $SCRIPTNAME requires doxygen to be installed" 1>&2
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then # Ubuntu
        echo "      :: try: apt-get install doxygen" 1>&2
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then # CentOS and RHEL and Fedora
        echo "      :: try: yum install doxygen" 1>&2
    elif [ "$DETECTEDOS" == "SuSE" ] ; then # SuSE
        echo "      :: try: zypper install doxygen" 1>&2
    else
        echo "      :: download from: http://doxygen.org" 1>&2
    fi
    echo "#######################################################" 1>&2
    exit 1
fi

HELP2MAN=`which help2man`
if [ "$?" -ne "0" ] ; then
    echo "#######################################################" 1>&2
    echo "ERROR :: $SCRIPTNAME requires help2man to be installed" 1>&2
    if [ "$DETECTEDOS" == "Ubuntu" ] ; then # Ubuntu
        echo "      :: try: apt-get install help2man" 1>&2
    elif [ "$DETECTEDOS" == "RedHatCompatible" ] ; then # CentOS and RHEL and Fedora
        echo "      :: try: yum install help2man" 1>&2
    elif [ "$DETECTEDOS" == "SuSE" ] ; then # SuSE
        echo "      :: try: zypper install help2man" 1>&2
    else
        echo "      :: download from: http://www.gnu.org/software/help2man/" 1>&2
    fi
    echo "#######################################################" 1>&2
    exit 1
else
    H2MVERSION=`help2man --version | head -n1 | awk '{print $3}'`
fi

# reset to exit on an error
set -e

# set up own temporary configfile
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

            echo "Detecting PostgreSQL Path: [$EIRODSPOSTGRESPATH]"
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
        echo "Switching context to [$BUILDDIR] for coverage-enabled build"
        # copy entire local tree to real package target location
        echo "Copying files into place..."
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
    sed -e "\,^IRODS_HOME,s,^.*$,IRODS_HOME=\`./scripts/find_irods_home.sh\`," ./irodsctl > /tmp/irodsctl.tmp
    mv /tmp/irodsctl.tmp ./irodsctl
    chmod 755 ./irodsctl

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

    # =-=-=-=-=-=-=-
    # modify the eirods_ms_home.h file with the proper path to the binary directory
    irods_msvc_home=`./scripts/find_irods_home.sh`
    irods_msvc_home="$irods_msvc_home/server/bin/"
    sed -e s,EIRODSMSVCPATH,$irods_msvc_home, ./server/re/include/eirods_ms_home.h.src > /tmp/eirods_ms_home.h
    mv /tmp/eirods_ms_home.h ./server/re/include/



    # find number of cpus
    if [ "$DETECTEDOS" == "MacOSX" ] ; then
        DETECTEDCPUCOUNT=`sysctl -n hw.ncpu`
    else
        DETECTEDCPUCOUNT=`cat /proc/cpuinfo | grep processor | wc -l`
    fi
    if [ "$DETECTEDCPUCOUNT" \< "2" ] ; then
        DETECTEDCPUCOUNT=1
    fi
    CPUCOUNT=$(( $DETECTEDCPUCOUNT + 3 ))
    MAKEJCMD="make -j $CPUCOUNT"
    echo "-------------------------------------"
    echo "Detected CPUs:    $DETECTEDCPUCOUNT"
    echo "Compiling with:   $MAKEJCMD"
    echo "-------------------------------------"
    sleep 1

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
    cd $BUILDDIR
    rst2pdf manual.rst -o manual.pdf
    if [ "$?" != "0" ] ; then
        echo "#######################################################" 1>&2
        echo "ERROR :: Failed generating manual.pdf" 1>&2
        echo "#######################################################" 1>&2
        exit 1
    fi
    # generate doxygen for microservices
    cd $BUILDDIR/iRODS
    doxygen ./config/doxygen-saved.cfg
    if [ "$?" != "0" ] ; then
        echo "#######################################################" 1>&2
        echo "ERROR :: Failed generating doxygen output" 1>&2
        echo "#######################################################" 1>&2
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
        echo "Building development package archive file..."
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

cd $BUILDDIR/epm
if [ "$BUILDEIRODS" == "1" ] ; then
    echo "Configuring EPM"
    set +e
    ./configure > /dev/null
    if [ "$?" != "0" ] ; then
        exit 1
    fi
    echo "Building EPM"
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
if [ "$DETECTEDOS" == "RedHatCompatible" ] ; then # CentOS and RHEL and Fedora
    echo "Running EPM :: Generating $DETECTEDOS RPMs"
    epmvar="REDHATRPM$SERVER_TYPE" 
    ./epm/epm $EPMOPTS -f rpm e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -f rpm e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -f rpm e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
elif [ "$DETECTEDOS" == "SuSE" ] ; then # SuSE
    echo "Running EPM :: Generating $DETECTEDOS RPMs"
    epmvar="SUSERPM$SERVER_TYPE" 
    ./epm/epm $EPMOPTS -f rpm e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -f rpm e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -f rpm e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
elif [ "$DETECTEDOS" == "Ubuntu" ] ; then  # Ubuntu
    echo "Running EPM :: Generating $DETECTEDOS DEBs"
    epmvar="DEB$SERVER_TYPE" 
    ./epm/epm $EPMOPTS -a amd64 -f deb e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -a amd64 -f deb e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -a amd64 -f deb e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
elif [ "$DETECTEDOS" == "Solaris" ] ; then  # Solaris
    echo "Running EPM :: Generating $DETECTEDOS PKGs"
    epmvar="PKG$SERVER_TYPE"
    ./epm/epm $EPMOPTS -f pkg e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -f pkg e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -f pkg e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
elif [ "$DETECTEDOS" == "MacOSX" ] ; then  # MacOSX
    echo "Running EPM :: Generating $DETECTEDOS DMGs"
    epmvar="OSX$SERVER_TYPE"
    ./epm/epm $EPMOPTS -f osx e-irods $epmvar=true ./packaging/e-irods.list
    if [ "$RELEASE" == "1" ] ; then
        ./epm/epm $EPMOPTS -f osx e-irods-icommands $epmvar=true ./packaging/e-irods-icommands.list
        ./epm/epm $EPMOPTS -f osx e-irods-dev $epmvar=true ./packaging/e-irods-dev.list
    fi
else
    echo "#######################################################" 1>&2
    echo "ERROR :: Unknown OS, cannot generate packages with EPM" 1>&2
    echo "#######################################################" 1>&2
    exit 1
fi


if [ "$COVERAGE" == "1" ] ; then
    # copy important bits back up
    echo "Copying generated packages back to original working directory..."
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


