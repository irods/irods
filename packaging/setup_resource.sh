#!/bin/bash

STATUS="firstpass"
echo "==================================================================="
echo ""
echo "You are installing an E-iRODS resource server.  Resource servers"
echo "cannot be started until they have been properly configured to"
echo "communicate with a live iCAT server."
echo ""
while [ "$STATUS" != "complete" ] ; do

  # set default values from an earlier loop
  if [ "$STATUS" != "firstpass" ] ; then
    LASTICATHOSTORIP=$ICATHOSTORIP
    LASTICATPORT=$ICATPORT
    LASTICATZONE=$ICATZONE
  fi

  # get host
  echo -n "iCAT server's hostname or IP address"
  if [ "$LASTICATHOSTORIP" ] ; then echo -n " [$LASTICATHOSTORIP]"; fi
  echo -n ": "
  read ICATHOSTORIP
  if [ "$ICATHOSTORIP" == "" ] ; then
    if [ "$LASTICATHOSTORIP" ] ; then ICATHOSTORIP=$LASTICATHOSTORIP; fi
  fi
  echo ""

  # get port
  echo -n "iCAT server's port"
  if [ "$LASTICATPORT" ] ; then
    echo -n " [$LASTICATPORT]"
  else
    echo -n " [1247]"
  fi
  echo -n ": "
  read ICATPORT
  if [ "$ICATPORT" == "" ] ; then
    if [ "$LASTICATPORT" ] ; then
      ICATPORT=$LASTICATPORT
    else
      ICATPORT=1247
    fi
  fi
  echo ""

  # get zone
  echo -n "iCAT server's ZoneName"
  if [ "$LASTICATZONE" ] ; then echo -n " [$LASTICATZONE]"; fi
  echo -n ": "
  read ICATZONE
  if [ "$ICATZONE" == "" ] ; then
    if [ "$LASTICATZONE" ] ; then ICATZONE=$LASTICATZONE; fi
  fi
  echo ""

  # confirm
  echo "-------------------------------------------"
  echo "Hostname or IP:   $ICATHOSTORIP"
  echo "iCAT Port:        $ICATPORT"
  echo "iCAT Zone:        $ICATZONE"
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
echo "==================================================================="

echo "Updating irods.config..."
sed -e "/^\$IRODS_ICAT_HOST/s/^.*$/\$IRODS_ICAT_HOST = '$ICATHOSTORIP';/" ./iRODS/config/irods.config > /tmp/tmp.irods.config
mv /tmp/tmp.irods.config ./iRODS/config/irods.config
sed -e "/^\$IRODS_PORT/s/^.*$/\$IRODS_PORT = '$ICATPORT';/" ./iRODS/config/irods.config > /tmp/tmp.irods.config
mv /tmp/tmp.irods.config ./iRODS/config/irods.config
sed -e "/^\$ZONE_NAME/s/^.*$/\$ZONE_NAME = '$ICATZONE';/" ./iRODS/config/irods.config > /tmp/tmp.irods.config
mv /tmp/tmp.irods.config ./iRODS/config/irods.config
new_resc_name="`hostname -s`Resource"
sed -e "/^\$RESOURCE_NAME/s/^.*$/\$RESOURCE_NAME = '$new_resc_name';/" ./iRODS/config/irods.config > /tmp/tmp.irods.config
mv /tmp/tmp.irods.config ./iRODS/config/irods.config


echo "Running eirods_setup.pl..."
cd iRODS
perl ./scripts/perl/eirods_setup.pl

