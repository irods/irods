#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../iRODS


export EIRODSPOSTGRESPATH=`../packaging/find_postgres.sh | sed -e s,\/[^\/]*$,, -e s,\/[^\/]*$,,`
export EIRODSPOSTGRESPATH="$EIRODSPOSTGRESPATH/"
echo "Detecting PostgreSQL Path: [$EIRODSPOSTGRESPATH]"

sed -e s,EIRODSPOSTGRESPATH,$EIRODSPOSTGRESPATH, ../packaging/irods.config.epm > /tmp/irods.config.epm

./scripts/configure
cp /tmp/irods.config.epm ./config/irods.config
./scripts/configure
cp /tmp/irods.config.epm ./config/irods.config
make

