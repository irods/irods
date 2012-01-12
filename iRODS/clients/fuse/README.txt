DIRECTORY
iRODS/clients/fuse	- iRODS/fuse implementation

DESCRIPTION
	This directory, and its subdirectories, contain modules for
	building the irodsFs binary which can be used to mount an
	iRODS collection to a local directory. Then the files and
        sub-collections in this mounted iRODS collection can be
	accessed using regular UNIX commands through this local directory.

FUSE (Filesystem in Userspace) is a free Unix kernel module that allows 
non-privileged users to create their own file systems without editing 
the kernel code. This is achieved by running the file system code in 
user space, while the FUSE module only provides a "bridge" to the actual 
kernel interfaces. 

The iRODS FUSE implementation allows normal users to access data stored 
in iRODS using standard UNIX commands (ls, cp, etc) and system calls 
(open, read, write, etc).

The user's iRODS passwd or credential will be used for authentication at 
the time of the mount. e.g., doing a iinit before the mount and he/she will 
be able to access all files/collections he/she is allowed to access under 
iRODS. The user will need to set the appropriate UNIX permission (mode) of 
the UNIX mount directory to control access to the mounted data.


Building irods FUSE:
--------------------

a) Edit the config/config.mk file:

1) Uncomment the line:
    # IRODS_FS = 1

2) set fuseHomeDir to the directory where the fuse library and include
files are installed. e.g.,

    fuseHomeDir=/usr/local/fuse

b) Making irods Fuse:

Type in:

cd clients/fuse
gmake

Running irods Fuse:
-------------------

1) cd clients/fuse/bin

2) make a local directory for mounting. e.g.,

    mkdir /usr/tmp/fmount

3) Setup the iRODS client env (~/irods/.irodsEnv) so that iCommands will
work. Type in:
    iinit

and do the normal login.

4) The 2.0 irodsFs caches small files in the local disk to improve 
performance. By default, the cached files are put in the /tmp/fuseCache
directory. The env variable "FuseCacheDir" can be used to change the
default cache directory. This env varible much be set before starting
irodsFs (step 5).

5) Mount the home collection to the local directory by typing in:
./irodsFs /usr/tmp/fmount

The user's home collection is now mounted. The iRODS files and sub-collections 
in the user's home collection should be accessible with normal UNIX commands 
through the /usr/tmp/fmount directory. 

Run irodsFs in debug mode
-------------------------
To run irodsFs in debug mode:

1) set the env variable irodsLogLevel to 4, e.g. in csh:

setenv irodsLogLevel 4

2) run irodsFs in debug mode, e.g.

irodsFs yourMountPoint -d

It should print out a lot of debugging info.


WARNING
-------

1) When a collection is mounted using irodsFs, users should not use iCommands
such as iput, irm, icp, etc to change the content of the collection because
the FUSE implementation seems to cache the attributes of the contents of
the collection.  

