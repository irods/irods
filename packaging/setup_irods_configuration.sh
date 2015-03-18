#!/bin/bash -e

# detect correct python version
if type -P python2 1> /dev/null; then
    PYTHON=`type -P python2`
else
    PYTHON=`type -P python`
fi

# find local working directory
DETECTEDDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

#
# Detect run-in-place installation
#
if [ -f "$DETECTEDDIR/binary_installation.flag" ] ; then
    RUNINPLACE=0
else
    RUNINPLACE=1
fi

# set some paths
if [ $RUNINPLACE -eq 1 ] ; then
    MYSERVERCONFIGJSON=$DETECTEDDIR/../iRODS/server/config/server_config.json
    MYICATSETUPVALUES=$DETECTEDDIR/../plugins/database/src/icatSetupValues.sql
    # clean full paths
    MYSERVERCONFIGJSON="$(cd "$( dirname $MYSERVERCONFIGJSON )" && pwd)"/"$( basename $MYSERVERCONFIGJSON )"
    if [ ! -f $MYSERVERCONFIGJSON ]; then
        echo ">>> Copying new server_config.json to $MYSERVERCONFIGJSON"
        cp $DETECTEDDIR/server_config.json $MYSERVERCONFIGJSON
    fi

    MYICATSETUPVALUES="$(cd "$( dirname $MYICATSETUPVALUES )" && pwd)"/"$( basename $MYICATSETUPVALUES )"
    DEFAULTRESOURCEDIR="$( cd "$( dirname "$( dirname "$DETECTEDDIR/../" )" )" && pwd )"/Vault
else
    MYSERVERCONFIGJSON=/etc/irods/server_config.json
    if [ ! -f $MYSERVERCONFIGJSON ]; then
        echo ">>> Copying new server_config.json to /etc/irods"
        cp $DETECTEDDIR/server_config.json $MYSERVERCONFIGJSON
    fi
    MYICATSETUPVALUES=/var/lib/irods/iRODS/server/icat/src/icatSetupValues.sql
    DEFAULTRESOURCEDIR=/var/lib/irods/iRODS/Vault
fi

# detect server type being installed
# resource server
ICAT_SERVER=0
if [ $RUNINPLACE -eq 0 -a -f $DETECTEDDIR/setup_irods_database.sh ] ; then
    # icat enabled server
    ICAT_SERVER=1
fi
if [ $RUNINPLACE -eq 1 ] ; then
    source $DETECTEDDIR/server_type.sh
    # icat enabled server
    if [ $SERVER_TYPE == "ICAT" ] ; then
        ICAT_SERVER=1
    fi
fi

    SETUP_IRODS_CONFIGURATION_FLAG="/tmp/$USER/setup_irods_configuration.flag"

    # get temp file from prior run, if it exists
    mkdir -p /tmp/$USER
    if [ -f $SETUP_IRODS_CONFIGURATION_FLAG ] ; then
        # have run this before, read the existing config files
        if [ $ICAT_SERVER -eq 1 ] ; then
            MYZONE=`$PYTHON -c "import json; print json.load(open('$MYSERVERCONFIGJSON'))['zone_name']"`
        fi
        MYPORT=`$PYTHON -c "import json; print json.load(open('$MYSERVERCONFIGJSON'))['zone_port']"`
        MYRANGESTART=`$PYTHON -c "import json; print json.load(open('$MYSERVERCONFIGJSON'))['server_port_range_start']"`
        MYRANGEEND=`$PYTHON -c "import json; print json.load(open('$MYSERVERCONFIGJSON'))['server_port_range_end']"`
        MYLOCALZONEID=`$PYTHON -c "import json; print json.load(open('$MYSERVERCONFIGJSON'))['zone_id']"`
        MYRESOURCEDIR=`$PYTHON -c "import json; print json.load(open('$MYSERVERCONFIGJSON'))['default_resource_directory']"`
        MYNEGOTIATIONKEY=`$PYTHON -c "import json; print json.load(open('$MYSERVERCONFIGJSON'))['negotiation_key']"`
        MYADMINNAME=`$PYTHON -c "import json; print json.load(open('$MYSERVERCONFIGJSON'))['zone_user']"`
        STATUS="loop"
    else
        # no temp file, this is the first run
        STATUS="firstpass"
    fi

    # strip cruft from zone_id
    tmp=${MYLOCALZONEID#\"}
    tmp=${tmp%\,}
    MYLOCALZONEID=${tmp%\"}

    # strip cruft from negotiation_key
    tmp=${MYNEGOTIATIONKEY#\"}
    tmp=${tmp%\,}
    MYNEGOTIATIONKEY=${tmp%\"}

    PREVIOUSID=$MYLOCALZONEID
    PREVIOUSKEY=$MYNEGOTIATIONKEY

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
        LASTMYZONE=$MYZONE
        LASTMYPORT=$MYPORT
        LASTMYRANGESTART=$MYRANGESTART
        LASTMYRANGEEND=$MYRANGEEND
        LASTMYRESOURCEDIR=$MYRESOURCEDIR
        LASTMYADMINNAME=$MYADMINNAME
        LASTMYLOCALZONEID=$MYLOCALZONEID
        LASTMYNEGOTIATIONKEY=$MYNEGOTIATIONKEY
      fi

      if [ $ICAT_SERVER -eq 1 ] ; then
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

      # get zone_id
      echo -n "iRODS server's zone_id"
      if [ "$LASTMYLOCALZONEID" ] ; then
        echo -n " [$LASTMYLOCALZONEID]"
      else
        echo -n " [TEMPORARY_zone_id]"
      fi
      echo -n ": "
      read MYLOCALZONEID
      if [ "$MYLOCALZONEID" == "" ] ; then
        if [ "$LASTMYLOCALZONEID" ] ; then
          MYLOCALZONEID=$LASTMYLOCALZONEID
        else
          MYLOCALZONEID="TEMPORARY_zone_id"
        fi
      fi
      # strip all forward slashes
      MYLOCALZONEID=`echo "${MYLOCALZONEID}" | sed -e "s/\///g"`
      echo ""

      # get negotiation_key
      NEGOTIATIONKEYLENGTH=0
      while [ $NEGOTIATIONKEYLENGTH -ne 32 ] ; do
          echo -n "iRODS server's negotiation_key"
          if [ "$LASTMYNEGOTIATIONKEY" ] ; then
            echo -n " [$LASTMYNEGOTIATIONKEY]"
          else
            echo -n " [TEMPORARY_32byte_negotiation_key]"
          fi
          echo -n ": "
          read MYNEGOTIATIONKEY
          if [ "$MYNEGOTIATIONKEY" == "" ] ; then
            if [ "$LASTMYNEGOTIATIONKEY" ] ; then
              MYNEGOTIATIONKEY=$LASTMYNEGOTIATIONKEY
            else
              MYNEGOTIATIONKEY="TEMPORARY_32byte_negotiation_key"
            fi
          fi
          # strip all forward slashes
          MYNEGOTIATIONKEY=`echo "${MYNEGOTIATIONKEY}" | sed -e "s/\///g"`
          echo ""
          # check length (must equal 32)
          NEGOTIATIONKEYLENGTH=${#MYNEGOTIATIONKEY}
          if [ $NEGOTIATIONKEYLENGTH -ne 32 ] ; then
              echo "   *** negotiation_key must be exactly 32 bytes ***"
              echo ""
              echo "   $MYNEGOTIATIONKEY <- $NEGOTIATIONKEYLENGTH bytes"
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

      if [ $ICAT_SERVER -eq 1 ] ; then
        echo -n "iRODS server's administrator password: "
        # get db password, without showing on screen
        read -s MYADMINPASSWORD
        echo ""
        echo ""
      fi

      # confirm
      echo "-------------------------------------------"
      if [ $ICAT_SERVER -eq 1 ] ; then
        echo "iRODS Zone:             $MYZONE"
      fi
      echo "iRODS Port:             $MYPORT"
      echo "Range (Begin):          $MYRANGESTART"
      echo "Range (End):            $MYRANGEEND"
      echo "Vault Directory:        $MYRESOURCEDIR"
      echo "zone_id:                $MYLOCALZONEID"
      echo "negotiation_key:        $MYNEGOTIATIONKEY"
      echo "Administrator Username: $MYADMINNAME"
      if [ $ICAT_SERVER -eq 1 ] ; then
        echo "Administrator Password: Not Shown"
      fi
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

    # update existing server_config.json
    echo "Updating $MYSERVERCONFIGJSON..."
    # zone name
    if [ $ICAT_SERVER -eq 1 ] ; then
        $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON string zone_name $MYZONE
    fi
    # everything else
    $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON string default_resource_directory $MYRESOURCEDIR
    $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON integer zone_port $MYPORT
    $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON integer server_port_range_start $MYRANGESTART
    $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON integer server_port_range_end $MYRANGEEND
    $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON string zone_user $MYADMINNAME
    $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON string zone_id $MYLOCALZONEID
    $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON string negotiation_key $MYNEGOTIATIONKEY
    $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON string icat_host `hostname`
    if [ $ICAT_SERVER -eq 1 ] ; then
        $PYTHON $DETECTEDDIR/update_json.py $MYSERVERCONFIGJSON string admin_password $MYADMINPASSWORD
    fi

    # prepare SQL from template
    if [ $ICAT_SERVER -eq 1 ] ; then
        echo "Preparing $MYICATSETUPVALUES..."
        TMPFILE="/tmp/$USER/icatsetupvalues.txt"
        MYHOSTNAME=`hostname`
        sed -e "s/ZONE_NAME_TEMPLATE/$MYZONE/g" $MYICATSETUPVALUES.template > $TMPFILE; mv $TMPFILE $MYICATSETUPVALUES
        sed -e "s/ADMIN_NAME_TEMPLATE/$MYADMINNAME/g" $MYICATSETUPVALUES > $TMPFILE; mv $TMPFILE $MYICATSETUPVALUES
        sed -e "s/HOSTNAME_TEMPLATE/$MYHOSTNAME/g" $MYICATSETUPVALUES > $TMPFILE; mv $TMPFILE $MYICATSETUPVALUES
        sed -e "s;RESOURCE_DIR_TEMPLATE;$MYRESOURCEDIR;g" $MYICATSETUPVALUES > $TMPFILE; mv $TMPFILE $MYICATSETUPVALUES
        sed -e "s/ADMIN_PASSWORD_TEMPLATE/$MYADMINPASSWORD/g" $MYICATSETUPVALUES > $TMPFILE; mv $TMPFILE $MYICATSETUPVALUES
        chmod 600 $MYICATSETUPVALUES
    fi
