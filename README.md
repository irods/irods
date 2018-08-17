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

## Build without any Package Repositories

If you need to build the iRODS iCommands without the use of any APT/YUM repositories, it will be necessary
to build all the dependencies yourself.  The steps include:

1. Download, build, and install packages from https://github.com/irods/externals
2. Update your `PATH` to include the newly built CMake
3. Download, build, and install `irods-dev(el)` and `irods-runtime` from https://github.com/irods/irods
4. Download, build, and install `irods-icommands` from https://github.com/irods/irods_client_icommands
   
Our dependency chain will shorten as older distributions age out.

The current setup supports new C++ features on those older distributions.

