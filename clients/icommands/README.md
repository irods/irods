# iRODS Client - iCommands

This repository hosts the client iCommands, the default command line interface to iRODS.

The client iCommands are released as a single package called `irods-icommands` in the repository at https://packages.irods.org/

Once the repository is configured, the `irods-icommands` package can be installed via the package manager on any supported OS.

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

## Userspace Packaging

A `userspace-tarball` buildsystem target is provided to generate a userspace tarball package. This package
will contain the iCommands and all required library dependencies.
See `packaging/userspace/build_and_package.example.sh` for an example of how to build and package the
iCommands for userspace deployment.

### Build Dependencies

The userspace packager needs a few extra packages to work properly:
- Required: Python 3.6+.
- Required: `setuptools` Python 3 package.
    - Available as `python3-setuptools` via yum/apt on Centos 7/Ubuntu.
    - Available as `setuptools` on PyPI.
- Recommended: `distro` Python 3 module.
    - Required for Python 3.8+.
    - Available as `python3-distro` via apt on Ubuntu 18.04+.
    - Available as `python36-distro` via yum on Centos 7.
    - Available as `distro` on PyPI.
- Recommended: `lief` Python 3 module, version 0.10.0+ (preferably 0.11.0+).
    - Available as `lief` on PyPI.
- Recommended: `chrpath` tool
    - Available as `chrpath` via yum/apt on Centos/Ubuntu.

If you've got `pip`, the following one-liner should get all the Python dependencies installed:
```sh
python3 -m pip install lief setuptools distro
```
