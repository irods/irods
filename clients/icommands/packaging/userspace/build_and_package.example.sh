#!/bin/bash
# Example build script. Tweak as needed.
set -e
set -x

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )" # directory containing this script

ICOMMANDS_SRC_DIR="$(readlink -s -m "${ICOMMANDS_SRC_DIR:-"${SCRIPTDIR}/../.."}")" # icommands source directory
ICOMMANDS_BLD_DIR="$(readlink -s -m "${ICOMMANDS_BLD_DIR:-"${ICOMMANDS_SRC_DIR}/../icommands_build/userspace"}")"

IRODS_SRC_DIR="$(readlink -s -m "${IRODS_SRC_DIR:-"${ICOMMANDS_SRC_DIR}/../irods_source"}")" # irods source directory
IRODS_BLD_DIR="$(readlink -s -m "${IRODS_BLD_DIR:-"${ICOMMANDS_BLD_DIR}/irods_build"}")" # irods build directory

IRODS_BUILD_PREFIX="$(readlink -s -m "${IRODS_BUILD_PREFIX:-"${IRODS_BLD_DIR}/pfx"}")" # temporary install prefix

QTY_CPUS="$(nproc --all)"
QTY_JOBS="${QTY_JOBS:-"$(( ( QTY_CPUS > 1 ? QTY_CPUS : 2 ) * 2 / 3 ))"}"

CMAKE="${CMAKE:-cmake}"
IRODS_BUILD_CONFIGURATION="${BUILD_CONFIGURATION:-Release}"

## Prepare workspace

if [ ! -d "${ICOMMANDS_BLD_DIR}" ]; then
	mkdir "${ICOMMANDS_BLD_DIR}"
fi

if [ ! -d "${IRODS_BLD_DIR}" ]; then
	mkdir "${IRODS_BLD_DIR}"
fi

if [ ! -d "${IRODS_BUILD_PREFIX}" ]; then
	mkdir "${IRODS_BUILD_PREFIX}"
fi


## Build iRODS

cd "${IRODS_BLD_DIR}"

"${CMAKE}" \
	-DCMAKE_BUILD_TYPE="${IRODS_BUILD_CONFIGURATION}" \
	-DCMAKE_INSTALL_PREFIX="${IRODS_BUILD_PREFIX}" \
	-DCMAKE_MODULE_LINKER_FLAGS="-Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs -Wl,-z,origin" \
	-DCMAKE_SHARED_LINKER_FLAGS="-Wl,--enable-new-dtags -Wl,--as-needed -Wl,-z,defs -Wl,-z,origin" \
	"${IRODS_SRC_DIR}"

make install -j"${QTY_JOBS}"


## Build and package iCommands

cd "${ICOMMANDS_BLD_DIR}"

"${CMAKE}" \
	-DCMAKE_BUILD_TYPE="${IRODS_BUILD_CONFIGURATION}" \
	-DCMAKE_INSTALL_PREFIX="${IRODS_BUILD_PREFIX}" \
	-DIRODS_DIR="${IRODS_BUILD_PREFIX}/lib/irods/cmake" \
	"${ICOMMANDS_SRC_DIR}"

make -j"${QTY_JOBS}" userspace-tarball
