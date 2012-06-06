#!/bin/bash

SCRIPTNAME=`basename $0`
BUILDEIRODS="1"

#read -d '' USAGE <<"EOF"
USAGE="
E-iRODS Build Script

Usage: $SCRIPTNAME [OPTIONS] <serverType> [databaseType]

Options:
 -s      Skip compilation of E-iRODS source.

Examples:
 $SCRIPTNAME icat postgres
 $SCRIPTNAME resource
 $SCRIPTNAME -s icat postgres
 $SCRIPTNAME -s resource
"

# boilerplate
echo "--------------------------------------"
echo " E-iRODS Build Script"
echo "--------------------------------------"

# parse options
while getopts ":hs" opt; do
  case $opt in
    h)
      echo "$USAGE"
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

# remove options from $@
shift $((OPTIND-1))

# check arguments
if [ $# -ne 1 -a $# -ne 2 ] ; then
  echo "$USAGE" 1>&2
  exit 1
fi


# detect operating system
UNAMERESULTS=`uname`
if [ -f "/etc/redhat-release" ]; then       # CentOS and RHEL and Fedora
    DETECTEDOS="RedHatCompatible" 
elif [ -f "/etc/SuSE-release" ]; then       # SuSE
    DETECTEDOS="SuSE" 
elif [ -f "/etc/lsb-release" ]; then        # Ubuntu
    DETECTEDOS="Ubuntu" 
elif [ -f "/usr/bin/sw_vers" ]; then        # MacOSX
    DETECTEDOS="MacOSX"
elif [ "$UNAMERESULTS" == "SunOS" ]; then   # Solaris
    DETECTEDOS="Solaris"
fi
echo "Detected OS [$DETECTEDOS]"


if [ $1 != "icat" -a $1 != "resource" ] ; then
    echo "ERROR :: Invalid serverType [$1]" 1>&2
    echo "      :: Only 'icat' or 'resource' available at this time" 1>&2
    exit 1
  exit 1
fi

if [ "$1" == "icat" ]; then
#  if [ "$2" != "postgres" -a "$2" != "mysql" ]
  if [ "$2" != "postgres" ]; then
    echo "ERROR :: Invalid iCAT databaseType [$2]" 1>&2
    echo "      :: Only 'postgres' available at this time" 1>&2
    exit 1
  fi
fi

if [ "$DETECTEDOS" == "Ubuntu" ]; then # Ubuntu
  if [ "$(id -u)" != "0" ]; then
    echo "ERROR :: $SCRIPTNAME must be run as root" 1>&2
    echo "      :: because dpkg demands to be run as root" 1>&2
    exit 1
  fi
fi

RST2PDF=`which rst2pdf`
if [ "$?" -ne "0" ]; then
  echo "ERROR :: $SCRIPTNAME requires rst2pdf to be installed" 1>&2
  if [ "$DETECTEDOS" == "Ubuntu" ]; then # Ubuntu
    echo "      :: try: apt-get install rst2pdf" 1>&2
  else
    echo "      :: try: easy_install rst2pdf" 1>&2
  fi
  exit 1
fi

ROMAN=`python -c "import roman"`
if [ "$?" -ne "0" ]; then
  echo "ERROR :: rst2pdf requires python module 'roman' to be installed" 1>&2
  echo "      :: try: easy_install roman" 1>&2
  exit 1
fi

DOXYGEN=`which doxygen`
if [ "$?" -ne "0" ]; then
  echo "ERROR :: $SCRIPTNAME requires doxygen to be installed" 1>&2
  if [ "$DETECTEDOS" == "Ubuntu" ]; then # Ubuntu
    echo "      :: try: apt-get install doxygen" 1>&2
  elif [ "$DETECTEDOS" == "RedHatCompatible" ]; then # CentOS and RHEL and Fedora
    echo "      :: try: yum install doxygen" 1>&2
  elif [ "$DETECTEDOS" == "SuSE" ]; then # SuSE
    echo "      :: try: zypper install doxygen" 1>&2
  else
    echo "      :: download from: http://doxygen.org" 1>&2
  fi
  exit 1
fi

HELP2MAN=`which help2man`
if [ "$?" -ne "0" ]; then
  echo "ERROR :: $SCRIPTNAME requires help2man to be installed" 1>&2
  if [ "$DETECTEDOS" == "Ubuntu" ]; then # Ubuntu
    echo "      :: try: apt-get install help2man" 1>&2
  elif [ "$DETECTEDOS" == "RedHatCompatible" ]; then # CentOS and RHEL and Fedora
    echo "      :: try: yum install help2man" 1>&2
  elif [ "$DETECTEDOS" == "SuSE" ]; then # SuSE
    echo "      :: try: zypper install help2man" 1>&2
  else
    echo "      :: download from: http://www.gnu.org/software/help2man/" 1>&2
  fi
  exit 1
fi


# get into the correct directory 
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../iRODS






# set up own temporary configfile
TMPCONFIGFILE=/tmp/$USER/irods.config.epm
mkdir -p $(dirname $TMPCONFIGFILE)



# set up variables for icat configuration
if [ $1 == "icat" ] ; then
    SERVER_TYPE="ICAT"
    DB_TYPE=$2
    EPMFILE="../packaging/irods.config.icat.epm"

    if [ "$BUILDEIRODS" == "1" ]; then
        # =-=-=-=-=-=-=-
        # bake SQL files for different database types
        # NOTE:: icatSysInserts.sql is handled by the packager as we rely on the default zone name
        serverSqlDir="./server/icat/src"
        convertScript="$serverSqlDir/convertSql.pl"

        echo "Converting SQL: [$convertScript] [$DB_TYPE] [$serverSqlDir]"
        `perl $convertScript $2 $serverSqlDir &> /dev/null`
        if [ "$?" -ne "0" ]; then
            echo "Failed to convert SQL forms" 1>&2
            exit 1
        fi

        # =-=-=-=-=-=-=-
        # insert postgres path into list file
        if [ "$DB_TYPE" == "postgres" ] ; then
            # need to do a dirname here, as the irods.config is expected to have a path
            # which will be appended with a /bin
            EIRODSPOSTGRESPATH=`../packaging/find_postgres_bin.sh`
            if [ "$EIRODSPOSTGRESPATH" == "FAIL" ]; then
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

if [ "$BUILDEIRODS" == "1" ]; then
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
    sed -e "s,SOMEPASSWORD,$RANDOMDBPASS," ./config/irods.config > /tmp/irods.config
    mv /tmp/irods.config ./config/irods.config

    # handle issue with IRODS_HOME being overwritten by the configure script    
    sed -e "\,^IRODS_HOME,s,^.*$,IRODS_HOME=\`./scripts/find_irods_home.sh\`," ./irodsctl > /tmp/irodsctl.tmp
    mv /tmp/irodsctl.tmp ./irodsctl
    chmod 755 ./irodsctl



    # =-=-=-=-=-=-=-
    # modify the eirods_ms_home.h file with the proper path to the binary directory
    irods_msvc_home=`./scripts/find_irods_home.sh`
    irods_msvc_home="$irods_msvc_home/server/bin/"
    sed -e s,EIRODSMSVCPATH,$irods_msvc_home, server/re/include/eirods_ms_home.h.src > /tmp/eirods_ms_home.h
    mv /tmp/eirods_ms_home.h server/re/include/




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
    make -j 4
    make -j 4
    if [ "$?" != "0" ]; then
     exit 1
    fi

    # generate randomized database password, replacing hardcoded placeholder
    cd $DIR/../
    sed -e "s,SOMEPASSWORD,$RANDOMDBPASS," ./packaging/e-irods.list.template > /tmp/eirodslist.tmp
    mv /tmp/eirodslist.tmp ./packaging/e-irods.list


    # generate manual in pdf format
    cd $DIR/../
    rst2pdf manual.rst -o manual.pdf
    if [ "$?" != "0" ]; then
      echo "ERROR :: Failed generating manual.pdf" 1>&2
      exit 1
    fi

    # generate doxygen for microservices
    cd $DIR/../iRODS
    doxygen ./config/doxygen-saved.cfg
    if [ "$?" != "0" ]; then
      echo "ERROR :: Failed generating doxygen output" 1>&2
      exit 1
    fi

fi # if $BUILDEIRODS


# prepare changelog for various platforms
cd $DIR/../
gzip -9 -c changelog > changelog.gz


# prepare man pages for every binary in the package
cd $DIR/../
MANDIR=man
rm -rf $MANDIR
mkdir -p $MANDIR
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
    igetwild.sh   
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
    help2man -h -h -N -n "an e-irods i-command" --version-string="E-iRODS-$EIRODSMANVERSION" $ICMDDIR/$ICMD > $MANDIR/$ICMD.1
done
for manfile in `ls $MANDIR`
do
    gzip -9 $MANDIR/$manfile
done







# run EPM for package type of this machine
# available from: http://fossies.org/unix/privat/epm-4.2-source.tar.gz
# md5sum 3805b1377f910699c4914ef96b273943

cd $DIR/../epm
echo "Configuring EPM"
./configure > /dev/null
if [ "$?" != "0" ]; then
 exit 1
fi
echo "Building EPM"
make > /dev/null
if [ "$?" != "0" ]; then
 exit 1
fi

cd $DIR/../
if [ "$DETECTEDOS" == "RedHatCompatible" ]; then # CentOS and RHEL and Fedora
  echo "Running EPM :: Generating $DETECTEDOS RPM"
  epmvar="REDHATRPM$SERVER_TYPE" 
  ./epm/epm -f rpm e-irods $epmvar=true ./packaging/e-irods.list
elif [ "$DETECTEDOS" == "SuSE" ]; then # SuSE
  echo "Running EPM :: Generating $DETECTEDOS RPM"
  epmvar="SUSERPM$SERVER_TYPE" 
  ./epm/epm -f rpm e-irods $epmvar=true ./packaging/e-irods.list
elif [ "$DETECTEDOS" == "Ubuntu" ]; then  # Ubuntu
  echo "Running EPM :: Generating $DETECTEDOS DEB"
  epmvar="DEB$SERVER_TYPE" 
  ./epm/epm -a amd64 -f deb e-irods $epmvar=true ./packaging/e-irods.list
elif [ "$DETECTEDOS" == "Solaris" ]; then  # Solaris
  echo "Running EPM :: Generating $DETECTEDOS PKG"
  epmvar="PKG$SERVER_TYPE"
  ./epm/epm -f pkg e-irods $epmvar=true ./packaging/e-irods.list
elif [ "$DETECTEDOS" == "MacOSX" ]; then  # MacOSX
  echo "Running EPM :: Generating $DETECTEDOS DMG"
  epmvar="OSX$SERVER_TYPE"
  ./epm/epm -f osx e-irods $epmvar=true ./packaging/e-irods.list
else
  echo "ERROR :: Unknown OS, cannot generate package with EPM" 1>&2
  exit 1
fi



