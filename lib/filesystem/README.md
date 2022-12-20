# iRODS Filesystem
An experimental implementation of the C++17 Filesystem library for iRODS.

It can be used on the client-side and the server-side.

This library does not support relative paths when interacting with iRODS objects. All paths must be **absolute** before any interaction with iRODS objects.

**API functions defined by the C++17 standard are safe to use however, functions introduced by the iRODS team are likely to experience change.**

## Using the Library
For usage examples, see the unit tests directory of this repository or visit the Library Examples section at [https://docs.irods.org](https://docs.irods.org).
