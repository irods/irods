# iRODS Filesystem
An experimental implementation of the C++17 Filesystem library for iRODS.

It can be used on both the client and server side.

This library does not support relative paths when interacting with iRODS objects. All paths must be absolute before any interaction with iRODS objects.

**This library is a work in progress and has NOT been fully tested!**

**API functions defined by the C++17 standard are safe to use however, functions introduced by the iRODS team are likely to experience change.**

## Requirements
- iRODS v4.2.6 or greater
- irods-dev package
- irods-runtime package
- irods-externals-boost package

## Using the Library
To use the library, simply include the filesystem header (e.g. `<irods/filesystem.hpp>`) and link against the appropriate shared library. Client-side software should link against `irods_client.so`. Server-side software should link against `irods_server.so`.

The library defaults to providing the client-side API. To enable the server-side API, define the following macro.
- IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

## TODOs
- Finish designing and implementing unit tests.
