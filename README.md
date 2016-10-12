# iRODS Client - iCommands

This repository hosts the client iCommands, the default command line interface to iRODS.

## Build

To build the iCommands, you will need the `irods-dev` and `irods-runtime` packages.

This is a CMake project and can be built with:

```
cd irods_client_icommands
mkdir build
cd build
cmake -GNinja ../
ninja package
```

## Install

The packages produced by CMake will install the ~50 iCommands, by default, into `/usr/bin`.
