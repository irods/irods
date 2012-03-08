#!/bin/bash

pushd iRODS
export EIRODSPOSTGRESVERSION=`\`which psql\` --version | head -n1 | awk '{print $3}' | cut -d'.' -f1,2`
echo "Detecting PostgreSQL Version: [$EIRODSPOSTGRESVERSION]"
sed -e s/EIRODSPOSTGRESVERSION/$EIRODSPOSTGRESVERSION/ ../packaging/irods.config.epm > /tmp/irods.config.epm
./scripts/configure
cp /tmp/irods.config.epm ./config/irods.config
./scripts/configure
cp /tmp/irods.config.epm ./config/irods.config
make
popd

