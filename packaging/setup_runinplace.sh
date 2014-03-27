#
# Detect run-in-place installation
#
if [ -f /etc/irods/irods.config ] ; then
    RUNINPLACE=0
else
    RUNINPLACE=1

    DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    MYIRODSCONFIG=$DETECTEDDIR/../iRODS/config/irods.config
    SETUP_RUNINPLACE_FLAG="/tmp/$USER/setup_runinplace.flag"
    DEFAULTRIPRESOURCEDIR="$( cd "$( dirname "$( dirname "$DETECTEDDIR/../" )" )" && pwd )"/Vault
    
    # get temp file from prior run, if it exists
    mkdir -p /tmp/$USER
    if [ -f $SETUP_RUNINPLACE_FLAG ] ; then
        # have run this before, read the existing config file
        RIPPORT=`grep "IRODS_PORT =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        RIPRANGESTART=`grep "SVR_PORT_RANGE_START =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        RIPRANGEEND=`grep "SVR_PORT_RANGE_END =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        RIPRESOURCEDIR=`grep "RESOURCE_DIR =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        RIPADMINNAME=`grep "IRODS_ADMIN_NAME =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        STATUS="loop"
    else
        # no temp file, this is the first run
        STATUS="firstpass"
    fi

    # ask human for irods environment
    echo "==================================================================="
    echo ""
    echo "You are installing iRODS with the --run-in-place option."
    echo ""
    echo "The iRODS server cannot be started until it has been configured."
    echo ""
    while [ "$STATUS" != "complete" ] ; do

      # set default values from an earlier loop
      if [ "$STATUS" != "firstpass" ] ; then
        LASTRIPPORT=$RIPPORT
        LASTRIPRANGESTART=$RIPRANGESTART
        LASTRANGEEND=$RIPRANGEEND
        LASTRESOURCEDIR=$RIPRESOURCEDIR
        LASTRIPADMINNAME=$RIPADMINNAME
      fi

      # get port
      echo -n "iRODS server's port"
      if [ "$LASTRIPPORT" ] ; then
        echo -n " [$LASTRIPPORT]"
      else
        echo -n " [1247]"
      fi
      echo -n ": "
      read RIPPORT
      if [ "$RIPPORT" == "" ] ; then
        if [ "$LASTRIPPORT" ] ; then
          RIPPORT=$LASTRIPPORT
        else
          RIPPORT="1247"
        fi
      fi
      # strip all forward slashes
      RIPPORT=`echo "${RIPPORT}" | sed -e "s/\///g"`
      echo ""

      # get the db name
      echo -n "iRODS port range (begin)"
      if [ "$LASTRIPRANGESTART" ] ; then
        echo -n " [$LASTRIPRANGESTART]"
      else
        echo -n " [20000]"
      fi
      echo -n ": "
      read RIPRANGESTART
      if [ "$RIPRANGESTART" == "" ] ; then
        if [ "$LASTRIPRANGESTART" ] ; then
          RIPRANGESTART=$LASTRIPRANGESTART
        else
          RIPRANGESTART="20000"
        fi
      fi
      # strip all forward slashes
      RIPRANGESTART=`echo "${RIPRANGESTART}" | sed -e "s/\///g"`
      echo ""

      # get database user
      echo -n "iRODS port range (end)"
      if [ "$LASTRANGEEND" ] ; then
        echo -n " [$LASTRANGEEND]"
      else
        echo -n " [20199]"
      fi
      echo -n ": "
      read RIPRANGEEND
      if [ "$RIPRANGEEND" == "" ] ; then
        if [ "$LASTRANGEEND" ] ; then
          RIPRANGEEND=$LASTRANGEEND
        else
          RIPRANGEEND="20199"
        fi
      fi
      # strip all forward slashes
      RIPRANGEEND=`echo "${RIPRANGEEND}" | sed -e "s/\///g"`
      echo ""

      # get resource directory for the vault
      echo -n "iRODS Vault directory"
      if [ "$LASTRIPRESOURCEDIR" ] ; then
        echo -n " [$LASTRIPRESOURCEDIR]"
      else
        echo -n " [$DEFAULTRIPRESOURCEDIR]"
      fi
      echo -n ": "
      read RIPRESOURCEDIR
      if [ "$RIPRESOURCEDIR" == "" ] ; then
        if [ "$LASTRIPRESOURCEDIR" ] ; then
          RIPRESOURCEDIR=$LASTRIPRESOURCEDIR
        else
          RIPRESOURCEDIR="$DEFAULTRIPRESOURCEDIR"
        fi
      fi
      echo ""

      # get admin name
      echo -n "iRODS server's adminstrator name"
      if [ "$LASTRIPADMINNAME" ] ; then
        echo -n " [$LASTRIPADMINNAME]"
      else
        echo -n " [rods]"
      fi
      echo -n ": "
      read RIPADMINNAME
      if [ "$RIPADMINNAME" == "" ] ; then
        if [ "$LASTRIPADMINNAME" ] ; then
          RIPADMINNAME=$LASTRIPADMINNAME
        else
          RIPADMINNAME="rods"
        fi
      fi
      # strip all forward slashes
      RIPADMINNAME=`echo "${RIPADMINNAME}" | sed -e "s/\///g"`
      echo ""

      echo -n "iRODS server's administrator password: "
      # get db password, without showing on screen
      read -s RIPADMINPASSWORD
      echo ""
      echo ""

      # confirm
      echo "-------------------------------------------"
      echo "iRODS Port:             $RIPPORT"
      echo "Range (Begin):          $RIPRANGESTART"
      echo "Range (End):            $RIPRANGEEND"
      echo "Vault Directory:        $RIPRESOURCEDIR"
      echo "Administrator Name:     $RIPADMINNAME"
      echo "Administrator Password: Not Shown"
      echo "-------------------------------------------"
      echo -n "Please confirm these settings [yes]: "
      read CONFIRM
      if [ "$CONFIRM" == "" -o "$CONFIRM" == "y" -o "$CONFIRM" == "Y" -o "$CONFIRM" == "yes" ]; then
        STATUS="complete"
      else
        STATUS="loop"
      fi
      echo ""
      echo ""

    done
    touch $SETUP_RUNINPLACE_FLAG
    
    
    # bolt onto the end of existing irods.config
    TMPFILE="/tmp/$USER/runinplaceirodsconfig.txt"
    echo "Updating $MYIRODSCONFIG..."
    sed -e "/^\$IRODS_PORT/s/^.*$/\$IRODS_PORT = '$RIPPORT';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep IRODS_PORT $MYIRODSCONFIG
    sed -e "/^\$SVR_PORT_RANGE_START/s/^.*$/\$SVR_PORT_RANGE_START = '$RIPRANGESTART';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep SVR_PORT_RANGE_START $MYIRODSCONFIG
    sed -e "/^\$SVR_PORT_RANGE_END/s/^.*$/\$SVR_PORT_RANGE_END = '$RIPRANGEEND';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep SVR_PORT_RANGE_END $MYIRODSCONFIG
    sed -e "s,^\$RESOURCE_DIR =.*$,\$RESOURCE_DIR = '$RIPRESOURCEDIR';," $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep RESOURCE_DIR $MYIRODSCONFIG
    sed -e "/^\$IRODS_ADMIN_NAME/s/^.*$/\$IRODS_ADMIN_NAME = '$RIPADMINNAME';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep IRODS_ADMIN_NAME $MYIRODSCONFIG
    sed -e "/^\$IRODS_ADMIN_PASSWORD/s/^.*$/\$IRODS_ADMIN_PASSWORD = '$RIPADMINPASSWORD';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep IRODS_ADMIN_PASSWORD $MYIRODSCONFIG
    
fi
