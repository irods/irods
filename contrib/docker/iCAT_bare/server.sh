#!/bin/bash
while read e; do
  export $e
done </var/lib/irods/env.tmp
cd /var/lib/irods/iRODS/server/bin
env PATH="$PATH:/var/lib/irods/iRODS/server/bin" ./irodsServer.actual -u
rm /var/lib/irods/env.tmp
