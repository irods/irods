# Distributing iCommands to Users

If your user community does not have root access to their machines, distributing a set of iCommands they can run is very desirable. This document discusses how to build, bundle and configure the iCommands for use by such a user.

## Building the iCommands

Starting from a clean irods repository, from within the top level directory run the command:

```
dan_admin:~/irods$ ./packaging/build.sh -r icommands
```

The build script will download the external libraries, compile the iRODS client library, and then compile the iCommands. This script will use EPM to package the iCommands into a deb or rpm file. However, we will ignore this package (since it can't be installed without root access) and instead use a second script to build a tar file containing all of the appropriate iCommand binaries and related client-side plugins. From the top level of the iRODS repository, run:

```
dan_admin:~/irods$ ./packaging/make_icommands_for_distribution.sh
```

This short script will create an 'icommands' directory and copy the iCommand binaries into it. It will create a plugins directory which will include the basic authentication and networking plugins, then tar it up for distribution to other users.

## Using the iCommands

Once the tar file has been expanded into a user's home directory the user will need to set the PATH to the iCommands and then set up the iRODS environment before authenticating against their Zone. Assuming the iCommands now reside in the home directory of joe_user, using a bash shell as an example the user needs to amend their PATH environment variable and verify that the iCommands are now in the path:

```
joe_user:~$ export PATH=/home/joe_user/icommands:$PATH
joe_user:~$ which ils
/home/joe_user/icommands/ils
```

Once the iCommands are successfully included in the user's PATH, the iRODS client environment needs to be bootstrapped. The user should make the iRODS environment directory and create an empty environment file:

```
joe_user:~$ mkdir .irods
joe_user:~$ touch .irods/irods_environment.json
```

Once this is done, the irods_environment.json file will need to be pointed towards the location of the client-side plugins in order for the `iinit` command to finish the bootstrapping process. Using their favorite editor the user needs to add a few lines to `.irods/irods_environment.json`:

```
{
  "irods_plugins_home" : "/home/joe_user/icommands/plugins/"
}
```

After the location of the plugins is made available to the iCommands, the user can run `iinit` to finish setting up the iRODS client environment:

```
joe_user:~$ iinit
One or more fields in your iRODS environment file (irods_environment.json) are
missing; please enter them.
Enter the host name (DNS) of the server to connect to: joeserver.example.org
Enter the port number: 1247
Enter your irods user name: joe
Enter your irods zone: joeZone
Those values will be added to your environment file (for use by
other iCommands) if the login succeeds.

Enter your current iRODS password:
```

At this point the user has a valid iRODS environment:

```
joe_user:~$ cat .irods/irods_environment.json
{
    "irods_host": "joeserver.example.org",
    "irods_plugins_home" : "/home/joe_user/icommands/plugins/",
    "irods_port": 1247,
    "irods_user_name": "joe",
    "irods_zone_name": "joeZone"
}
```

Joe can now begin using the iCommands:

```
joe_user:~$ ils
/joeZone/home/joe:
```

It is also important to remember there are many other additional parameters available for configuration:

  - [https://github.com/irods/irods_schema_configuration](https://github.com/irods/irods_schema_configuration)
