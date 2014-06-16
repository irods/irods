#
# Detect run-in-place installation
#
if [ -f /etc/irods/irods.config ] ; then
    RUNINPLACE=0
else
    RUNINPLACE=1
fi



if [ $RUNINPLACE -eq 1 ] ; then
    DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    MYIRODSCONFIG=$DETECTEDDIR/../iRODS/config/irods.config
    MYSERVERCONFIG=$DETECTEDDIR/../iRODS/server/config/server.config
    MYICATSYSINSERTS=$DETECTEDDIR/../iRODS/server/icat/src/icatSysInserts.sql
    # clean full paths
    MYIRODSCONFIG="$(cd "$( dirname $MYIRODSCONFIG )" && pwd)"/"$( basename $MYIRODSCONFIG )"
    MYSERVERCONFIG="$(cd "$( dirname $MYSERVERCONFIG )" && pwd)"/"$( basename $MYSERVERCONFIG )"
    MYICATSYSINSERTS="$(cd "$( dirname $MYICATSYSINSERTS )" && pwd)"/"$( basename $MYICATSYSINSERTS )"
    DEFAULTRESOURCEDIR="$( cd "$( dirname "$( dirname "$DETECTEDDIR/../" )" )" && pwd )"/Vault
else
    MYIRODSCONFIG=/etc/irods/irods.config
    MYSERVERCONFIG=/etc/irods/server.config
    MYICATSYSINSERTS=/var/lib/irods/iRODS/server/icat/src/icatSysInserts.sql
    DEFAULTRESOURCEDIR=/var/lib/irods/iRODS/Vault
fi

    SETUP_IRODS_CONFIGURATION_FLAG="/tmp/$USER/setup_irods_configuration.flag"

    # get temp file from prior run, if it exists
    mkdir -p /tmp/$USER
    if [ -f $SETUP_IRODS_CONFIGURATION_FLAG ] ; then
        # have run this before, read the existing config files
        MYPORT=`grep "IRODS_PORT =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        MYZONE=`grep "ZONE_NAME =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        MYRANGESTART=`grep "SVR_PORT_RANGE_START =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        MYRANGEEND=`grep "SVR_PORT_RANGE_END =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        MYRESOURCEDIR=`grep "RESOURCE_DIR =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        MYLOCALZONESID=`grep "^LocalZoneSID " $MYSERVERCONFIG | awk '{print $2}'`
        MYAGENTKEY=`grep "^agent_key " $MYSERVERCONFIG | awk '{print $2}'`
        MYADMINNAME=`grep "IRODS_ADMIN_NAME =" $MYIRODSCONFIG | awk -F\' '{print $2}'`
        STATUS="loop"
    else
        # no temp file, this is the first run
        STATUS="firstpass"
    fi

    # ask human for irods environment
    echo "==================================================================="
    echo ""
    if [ $RUNINPLACE -eq 1 ] ; then
        echo "You are installing iRODS with the --run-in-place option."
    else
        echo "You are installing iRODS."
    fi
    echo ""
    echo "The iRODS server cannot be started until it has been configured."
    echo ""
    while [ "$STATUS" != "complete" ] ; do

      # set default values from an earlier loop
      if [ "$STATUS" != "firstpass" ] ; then
        LASTMYPORT=$MYPORT
        LASTMYZONE=$MYZONE
        LASTMYRANGESTART=$MYRANGESTART
        LASTMYRANGEEND=$MYRANGEEND
        LASTMYRESOURCEDIR=$MYRESOURCEDIR
        LASTMYADMINNAME=$MYADMINNAME
        LASTMYLOCALZONESID=$MYLOCALZONESID
        LASTMYAGENTKEY=$MYAGENTKEY
      fi

      # get port
      echo -n "iRODS server's port"
      if [ "$LASTMYPORT" ] ; then
        echo -n " [$LASTMYPORT]"
      else
        echo -n " [1247]"
      fi
      echo -n ": "
      read MYPORT
      if [ "$MYPORT" == "" ] ; then
        if [ "$LASTMYPORT" ] ; then
          MYPORT=$LASTMYPORT
        else
          MYPORT="1247"
        fi
      fi
      # strip all forward slashes
      MYPORT=`echo "${MYPORT}" | sed -e "s/\///g"`
      echo ""

      # get zone
      echo -n "iRODS server's zone name"
      if [ "$LASTMYZONE" ] ; then
        echo -n " [$LASTMYZONE]"
      else
        echo -n " [tempZone]"
      fi
      echo -n ": "
      read MYZONE
      if [ "$MYZONE" == "" ] ; then
        if [ "$LASTMYZONE" ] ; then
          MYZONE=$LASTMYZONE
        else
          MYZONE="tempZone"
        fi
      fi
      # strip all forward slashes
      MYZONE=`echo "${MYZONE}" | sed -e "s/\///g"`
      echo ""

      # get the db name
      echo -n "iRODS port range (begin)"
      if [ "$LASTMYRANGESTART" ] ; then
        echo -n " [$LASTMYRANGESTART]"
      else
        echo -n " [20000]"
      fi
      echo -n ": "
      read MYRANGESTART
      if [ "$MYRANGESTART" == "" ] ; then
        if [ "$LASTMYRANGESTART" ] ; then
          MYRANGESTART=$LASTMYRANGESTART
        else
          MYRANGESTART="20000"
        fi
      fi
      # strip all forward slashes
      MYRANGESTART=`echo "${MYRANGESTART}" | sed -e "s/\///g"`
      echo ""

      # get database user
      echo -n "iRODS port range (end)"
      if [ "$LASTMYRANGEEND" ] ; then
        echo -n " [$LASTMYRANGEEND]"
      else
        echo -n " [20199]"
      fi
      echo -n ": "
      read MYRANGEEND
      if [ "$MYRANGEEND" == "" ] ; then
        if [ "$LASTMYRANGEEND" ] ; then
          MYRANGEEND=$LASTMYRANGEEND
        else
          MYRANGEEND="20199"
        fi
      fi
      # strip all forward slashes
      MYRANGEEND=`echo "${MYRANGEEND}" | sed -e "s/\///g"`
      echo ""

      # get resource directory for the vault
      echo -n "iRODS Vault directory"
      if [ "$LASTMYRESOURCEDIR" ] ; then
        echo -n " [$LASTMYRESOURCEDIR]"
      else
        echo -n " [$DEFAULTRESOURCEDIR]"
      fi
      echo -n ": "
      read MYRESOURCEDIR
      if [ "$MYRESOURCEDIR" == "" ] ; then
        if [ "$LASTMYRESOURCEDIR" ] ; then
          MYRESOURCEDIR=$LASTMYRESOURCEDIR
        else
          MYRESOURCEDIR="$DEFAULTRESOURCEDIR"
        fi
      fi
      echo ""

      # get LocalZoneSID
      echo -n "iRODS server's LocalZoneSID"
      if [ "$LASTMYLOCALZONESID" ] ; then
        echo -n " [$LASTMYLOCALZONESID]"
      else
        echo -n " [TEMP_LOCAL_ZONE_SID]"
      fi
      echo -n ": "
      read MYLOCALZONESID
      if [ "$MYLOCALZONESID" == "" ] ; then
        if [ "$LASTMYLOCALZONESID" ] ; then
          MYLOCALZONESID=$LASTMYLOCALZONESID
        else
          MYLOCALZONESID="TEMP_LOCAL_ZONE_SID"
        fi
      fi
      # strip all forward slashes
      MYLOCALZONESID=`echo "${MYLOCALZONESID}" | sed -e "s/\///g"`
      echo ""

      # get agent_key
      AGENTKEYLENGTH=0
      while [ $AGENTKEYLENGTH -ne 32 ] ; do
          echo -n "iRODS server's agent_key"
          if [ "$LASTMYAGENTKEY" ] ; then
            echo -n " [$LASTMYAGENTKEY]"
          else
            echo -n " [temp_32_byte_key_for_agent__conn]"
          fi
          echo -n ": "
          read MYAGENTKEY
          if [ "$MYAGENTKEY" == "" ] ; then
            if [ "$LASTMYAGENTKEY" ] ; then
              MYAGENTKEY=$LASTMYAGENTKEY
            else
              MYAGENTKEY="temp_32_byte_key_for_agent__conn"
            fi
          fi
          # strip all forward slashes
          MYAGENTKEY=`echo "${MYAGENTKEY}" | sed -e "s/\///g"`
          echo ""
          # check length (must equal 32)
          AGENTKEYLENGTH=${#MYAGENTKEY}
          if [ $AGENTKEYLENGTH -ne 32 ] ; then
              echo "   *** agent_key must be exactly 32 bytes ***"
              echo ""
              echo "   $MYAGENTKEY <- $AGENTKEYLENGTH bytes"
              echo "   ________________________________ <- 32 bytes"
              echo ""
          fi
      done

      # get admin name
      echo -n "iRODS server's administrator username"
      if [ "$LASTMYADMINNAME" ] ; then
        echo -n " [$LASTMYADMINNAME]"
      else
        echo -n " [rods]"
      fi
      echo -n ": "
      read MYADMINNAME
      if [ "$MYADMINNAME" == "" ] ; then
        if [ "$LASTMYADMINNAME" ] ; then
          MYADMINNAME=$LASTMYADMINNAME
        else
          MYADMINNAME="rods"
        fi
      fi
      # strip all forward slashes
      MYADMINNAME=`echo "${MYADMINNAME}" | sed -e "s/\///g"`
      echo ""

      echo -n "iRODS server's administrator password: "
      # get db password, without showing on screen
      read -s MYADMINPASSWORD
      echo ""
      echo ""

      # confirm
      echo "-------------------------------------------"
      echo "iRODS Port:             $MYPORT"
      echo "iRODS Zone:             $MYZONE"
      echo "Range (Begin):          $MYRANGESTART"
      echo "Range (End):            $MYRANGEEND"
      echo "Vault Directory:        $MYRESOURCEDIR"
      echo "LocalZoneSID:           $MYLOCALZONESID"
      echo "agent_key:              $MYAGENTKEY"
      echo "Administrator Username: $MYADMINNAME"
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
    touch $SETUP_IRODS_CONFIGURATION_FLAG


    # update existing irods.config
    TMPFILE="/tmp/$USER/setupirodsconfig.txt"
    echo "Updating $MYIRODSCONFIG..."
    sed -e "/^\$IRODS_PORT/s/^.*$/\$IRODS_PORT = '$MYPORT';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep IRODS_PORT $MYIRODSCONFIG
    sed -e "/^\$ZONE_NAME/s/^.*$/\$ZONE_NAME = '$MYZONE';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep ZONE_NAME $MYIRODSCONFIG
    sed -e "/^\$SVR_PORT_RANGE_START/s/^.*$/\$SVR_PORT_RANGE_START = '$MYRANGESTART';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep SVR_PORT_RANGE_START $MYIRODSCONFIG
    sed -e "/^\$SVR_PORT_RANGE_END/s/^.*$/\$SVR_PORT_RANGE_END = '$MYRANGEEND';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep SVR_PORT_RANGE_END $MYIRODSCONFIG
    sed -e "s,^\$RESOURCE_DIR =.*$,\$RESOURCE_DIR = '$MYRESOURCEDIR';," $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep RESOURCE_DIR $MYIRODSCONFIG
    sed -e "/^\$IRODS_ADMIN_NAME/s/^.*$/\$IRODS_ADMIN_NAME = '$MYADMINNAME';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep IRODS_ADMIN_NAME $MYIRODSCONFIG
    sed -e "/^\$IRODS_ADMIN_PASSWORD/s/^.*$/\$IRODS_ADMIN_PASSWORD = '$MYADMINPASSWORD';/" $MYIRODSCONFIG > $TMPFILE ; mv $TMPFILE $MYIRODSCONFIG
#    grep IRODS_ADMIN_PASSWORD $MYIRODSCONFIG

    # updating SQL
    TMPFILE="/tmp/$USER/setupicatsysinserts.txt"
    echo "Updating $MYICATSYSINSERTS..."
    if [ $(grep -c "'tempZone'" $MYICATSYSINSERTS) -eq 0 ] ; then
        echo "====================================="
        echo "ERROR:"
        echo "Unknown existing Zone name in $MYICATSYSINSERTS."
        echo "Please drop all tables and try again."
        echo "====================================="
        # restore original
        cp $MYICATSYSINSERTS.orig $MYICATSYSINSERTS
        exit 1
    fi
    if [ "$LASTMYADMINNAME" != "" -a "$LASTMYADMINNAME" != "rods" -a "$LASTMYADMINNAME" != "$MYADMINNAME" ] ; then
        echo "====================================="
        echo "ERROR:"
        echo "Cannot change existing non-default administrator username."
        echo ""
        echo "Please:"
        echo "1) Drop all of the iCAT tables,"
        echo "2) Reset $MYIRODSCONFIG with \$IRODS_ADMIN_NAME = 'rods';, and"
        echo "3) Run this script again."
        echo "====================================="
        exit 1
    fi
    # store original
    cp $MYICATSYSINSERTS $MYICATSYSINSERTS.orig
    # substitute
    sed -e "s/'tempZone'/'$MYZONE'/" $MYICATSYSINSERTS > $TMPFILE ; mv $TMPFILE $MYICATSYSINSERTS

    # update existing server.config
    TMPFILE="/tmp/$USER/setupserverconfig.txt"
    echo "Updating $MYSERVERCONFIG..."
    if [ $(grep -c "^LocalZoneSID " $MYSERVERCONFIG) -eq 0 ] ; then
        echo "LocalZoneSID DEFAULT" >> $MYSERVERCONFIG
    fi
    sed -e "/^LocalZoneSID /s/^.*$/LocalZoneSID $MYLOCALZONESID/" $MYSERVERCONFIG > $TMPFILE ; mv $TMPFILE $MYSERVERCONFIG
    if [ $(grep -c "^agent_key " $MYSERVERCONFIG) -eq 0 ] ; then
        echo "agent_key DEFAULT" >> $MYSERVERCONFIG
    fi
    sed -e "/^agent_key /s/^.*$/agent_key $MYAGENTKEY/" $MYSERVERCONFIG > $TMPFILE ; mv $TMPFILE $MYSERVERCONFIG
