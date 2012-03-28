#!/bin/bash

echo "==================================================================="
echo ""
echo "You have installed an E-iRODS resource server.  Resource servers"
echo "cannot be started until they have been properly configured to"
echo "communicate with a live iCAT server."
echo ""
echo -n "iCAT server's hostname or IP address: "
read ICATHOSTORIP
echo ""
echo -n "iCAT server's port [1247]: "
read ICATPORT
if [ "$ICATPORT" == "" ]; then
  ICATPORT=1247
fi
echo ""
echo -n "iCAT server's ZoneName: "
read ICATZONE
echo ""
echo "-------------------"
echo "Hostname or IP: [$ICATHOSTORIP]"
echo "iCAT Port:      [$ICATPORT]"
echo "iCAT Zone:      [$ICATZONE]"
echo "==================================================================="
