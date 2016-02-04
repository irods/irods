# Installation

iRODS is provided in binary form in a collection of interdependent packages.  There are two types of iRODS server, iCAT and Resource:

1. An iCAT server manages a Zone, handles the database connection to the iCAT metadata catalog (which could be either local or remote), and can provide [Storage Resources](architecture.md#storage-resources).  An iRODS Zone will have exactly one iCAT server.
2. A Resource server connects to an existing Zone and can provide additional storage resource(s).  An iRODS Zone can have zero or more Resource servers.

An iCAT server is just a Resource server that also provides the central point of coordination for the Zone and manages the metadata.

A single computer cannot have both an iCAT server and a Resource server installed.

The simplest iRODS installation consists of one iCAT server and zero Resource servers.

## iCAT Server

The irods-icat package installs the iRODS binaries and management scripts.

Additionally, an iRODS database plugin is required. iRODS uses this plugin (see [Pluggable Database](architecture.md#pluggable-database) to communicate with the desired database management system (e.g. Oracle, MySQL, PostgreSQL).

The iRODS installation script (which also configures the iRODS database plugin) requires database connection information about an existing database.  iRODS neither creates nor manages a database instance itself, just the tables within the database. Therefore, the database instance should be created and configured before installing iRODS.

### Database Setup

iRODS can use many different database configurations.  Local database examples are included below:

#### PostgreSQL on Ubuntu 14.04:

~~~
$ (sudo) su - postgres
postgres$ psql
psql> CREATE USER irods WITH PASSWORD 'testpassword';
psql> CREATE DATABASE "ICAT";
psql> GRANT ALL PRIVILEGES ON DATABASE "ICAT" TO irods;
~~~

Confirmation of the permissions can be viewed with ``\l`` within the ``psql`` console:

~~~
 psql> \l
                                   List of databases
    Name    |  Owner   | Encoding |   Collate   |    Ctype    |   Access privileges
 -----------+----------+----------+-------------+-------------+-----------------------
  ICAT      | postgres | UTF8     | en_US.UTF-8 | en_US.UTF-8 | =Tc/postgres         +
            |          |          |             |             | postgres=CTc/postgres+
            |          |          |             |             | irods=CTc/postgres
 ...
 (N rows)
~~~

#### MySQL on Ubuntu 14.04:

~~~
$ mysql
mysql> CREATE DATABASE ICAT;
mysql> CREATE USER 'irods'@'localhost' IDENTIFIED BY 'testpassword';
mysql> GRANT ALL ON ICAT.* to 'irods'@'localhost';
~~~

Confirmation of the permissions can be viewed:

~~~
mysql> SHOW GRANTS FOR 'irods'@'localhost';
+--------------------------------------------------------------------------------------------------------------+
| Grants for irods@localhost                                                                                   |
+--------------------------------------------------------------------------------------------------------------+
| GRANT USAGE ON *.* TO 'irods'@'localhost' IDENTIFIED BY PASSWORD '*9F69E47E519D9CA02116BF5796684F7D0D45F8FA' |
| GRANT ALL PRIVILEGES ON `ICAT`.* TO 'irods'@'localhost'                                                      |
+--------------------------------------------------------------------------------------------------------------+
N rows in set (0.00 sec)

~~~

### iRODS Setup

Installation of the iCAT DEB and PostgreSQL plugin DEB:

~~~
$ (sudo) dpkg -i irods-icat-TEMPLATE_IRODSVERSION-ubuntu14-x86_64.deb irods-database-plugin-postgres-1.8-ubuntu14-x86_64.deb
$ (sudo) apt-get -f install
~~~

Once the PostgreSQL database plugin has been installed, the following text will be displayed:

~~~
=======================================================================

iRODS Postgres Database Plugin installation was successful.

To configure this plugin, the following prerequisites need to be met:
 - an existing database user (to be used by the iRODS server)
 - an existing database (to be used as the iCAT catalog)
 - permissions for existing user on existing database

Please run the following setup script:
  sudo /var/lib/irods/packaging/setup_irods.sh

=======================================================================
~~~

The `setup_irods.sh` script below will prompt for, and then create, if necessary, a service account and service group which will own and operate the iRODS server:

~~~
$ (sudo) /var/lib/irods/packaging/setup_irods.sh
~~~

The `setup_irods.sh` script will ask for the following nineteen pieces of information before starting the iRODS server:

1. Service Account Name
2. Service Account Group
3. iCAT Zone
4. iCAT Port
5. Parallel Port Range (Begin)
6. Parallel Port Range (End)
7. Vault Directory
8. zone_key
9. negotiation_key
10. Control Plane Port
11. Control Plane Key
12. Schema Validation Base URI
13. iRODS Administrator Username
14. iRODS Administrator Password
15. Database Server's Hostname or IP
16. Database Port
17. Database Name
18. Database User
19. Database Password

!!! Note
    A default system PostgreSQL installation does not listen on a TCP port, it only listens on a local socket.  If your PostgreSQL server is localhost, use 'localhost' for 15) above.

!!! Note
    A default system PostgreSQL installation is configured for ident-based authentication which means the unix service account name must match the database user name.  iRODS currently assumes this is the case.

!!! Note
    Installing the MySQL database plugin will also require [Installing lib_mysqludf_preg](architecture.md#installing-lib_mysqludf_preg).  These functions are required for the internal iRODS SQL which uses regular expressions.

!!! Note
    When running iRODS on pre-8.4 PostgreSQL, it is necessary to remove an optimized specific query which was not yet available:

~~~
irods@hostname:~/ $ iadmin rsq DataObjInCollReCur
~~~

## Resource Server

The irods-resource package installs the iRODS binaries and management scripts.

There are no required additional packages, but the administrator will need to run a short setup script that will prompt for iRODS connection information and configure the server.

The `setup_irods.sh` script below will prompt for, and then create if necessary, a service account and service group which will own and operate the iRODS server.

Installation of the Resource RPM:

~~~
- Make sure to read ./packaging/RPM_INSTALLATION_HOWTO.txt before trying to install the RPM package.
 $ (sudo) rpm -i irods-resource-TEMPLATE_IRODSVERSION-centos6-x86_64.rpm
 $ (sudo) /var/lib/irods/packaging/setup_irods.sh
~~~

The `setup_irods.sh` script will ask for the following fifteen pieces of information about the existing Zone that the iRODS resource server will need in order to stand up and then connect to its configured iCAT Zone:

1. Service Account Name
2. Service Account Group
3. iCAT Port
4. Parallel Port Range (Begin)
5. Parallel Port Range (End)
6. Vault Directory
7. zone_key
8. negotiation_key
9. Control Plane Port
10. Control Plane Key
11. Schema Validation Base URI
12. iRODS Administrator Username
13. iCAT Host
14. iCAT Zone
15. iRODS Administrator Password

## Default Environment

Once a server is up and running, the default environment can be shown:

~~~
irods@hostname:~/ $ ienv
NOTICE: Release Version = rodsTEMPLATE_IRODSVERSION, API Version = d
NOTICE: irods_session_environment_file - /var/lib/irods/.irods/irods_environment.json.19345
NOTICE: irods_user_name - rods
NOTICE: irods_host - hostname
NOTICE: xmsg_host is not defined
NOTICE: irods_home - /tempZone/home/rods
NOTICE: irods_cwd - /tempZone/home/rods
NOTICE: irods_authentication_scheme is not defined
NOTICE: irods_port - 1247
NOTICE: xmsg_port is not defined
NOTICE: irods_default_resource - demoResc
NOTICE: irods_zone_name - tempZone
NOTICE: irods_client_server_policy - CS_NEG_REFUSE
NOTICE: irods_client_server_negotiation - request_server_negotiation
NOTICE: irods_encryption_key_size - 32
NOTICE: irods_encryption_salt_size - 8
NOTICE: irods_encryption_num_hash_rounds - 16
NOTICE: irods_encryption_algorithm - AES-256-CBC
NOTICE: irods_default_hash_scheme - SHA256
NOTICE: irods_match_hash_policy - compatible
NOTICE: irods_gsi_server_dn is not defined
NOTICE: irods_debug is not defined
NOTICE: irods_log_level is not defined
NOTICE: irods_authentication_file is not defined
NOTICE: irods_ssl_ca_certificate_path is not defined
NOTICE: irods_ssl_ca_certificate_file is not defined
NOTICE: irods_ssl_verify_server is not defined
NOTICE: irods_ssl_certificate_chain_file is not defined
NOTICE: irods_ssl_certificate_key_file is not defined
NOTICE: irods_ssl_dh_params_file is not defined
NOTICE: irods_server_control_plane_key - TEMPORARY__32byte_ctrl_plane_key
NOTICE: irods_server_control_plane_encryption_num_hash_rounds - 16
NOTICE: irods_server_control_plane_encryption_algorithm - AES-256-CBC
NOTICE: irods_server_control_plane_port - 1248
NOTICE: irods_maximum_size_for_single_buffer_in_megabytes - 32
NOTICE: irods_default_number_of_transfer_threads - 4
NOTICE: irods_transfer_buffer_size_for_parallel_transfer_in_megabytes - 4
NOTICE: irods_plugins_home is not defined
~~~

## Run In Place

iRODS can be compiled from source and run from the same directory.  Although this is not recommended for production deployment, it may be useful for testing, running multiple iRODS servers on the same system, running iRODS on systems without a package manager, and users who do not have administrator rights on their system.

To run iRODS in place, the build script must be called with the appropriate flag:

~~~
user@hostname:~/irods/ $ ./packaging/build.sh --run-in-place icat postgres
~~~

iRODS has several build dependencies (external libraries) which need to be in place before iRODS itself can be compiled successfully.  The following sequence should build the dependencies from source (in `irods/external/`) and guarantee reproducible builds:

~~~
# Populate configuration files (and fail to build)
user@hostname:~/irods/ $ ./packaging/build.sh --run-in-place icat postgres

# Rebuild the external libraries from source
user@hostname:~/irods/ $ cd external
user@hostname:~/irods/external/ $ make clean
user@hostname:~/irods/external/ $ make generate

# Build iRODS
user@hostname:~/irods/external/ $ cd ..
user@hostname:~/irods/ $ ./packaging/build.sh clean
user@hostname:~/irods/ $ ./packaging/build.sh --run-in-place icat postgres
~~~

After the system is built, the setup_irods_database.sh script needs to be run from its original location:

~~~
user@hostname:~/irods/ $ ./plugins/database/packaging/setup_irods_database.sh
~~~

The script will prompt for iRODS configuration information that would already be known to a binary installation:

~~~
===================================================================

You are installing iRODS with the --run-in-place option.

The iRODS server cannot be started until it has been configured.

iRODS server's zone name [tempZone]:

iRODS server's port [1247]:

iRODS port range (begin) [20000]:

iRODS port range (end) [20199]:

iRODS Vault directory [/full/path/to/Vault]:

iRODS server's zone_key [TEMPORARY_zone_key]:

iRODS server's negotiation_key [TEMPORARY_32byte_negotiation_key]:

Control Plane port [1248]:

Control Plane key [TEMPORARY__32byte_ctrl_plane_key]:

Schema Validation Base URI (or 'off') [https://schemas.irods.org/configuration]:

iRODS server's administrator username [rods]:

iRODS server's administrator password:

-------------------------------------------
iRODS Zone:                 tempZone
iRODS Port:                 1247
Range (Begin):              20000
Range (End):                20199
Vault Directory:            /full/path/to/Vault
zone_key:                   TEMPORARY_zone_key
negotiation_key:            TEMPORARY_32byte_negotiation_key
Control Plane Port:         1248
Control Plane Key:          TEMPORARY__32byte_ctrl_plane_key
Schema Validation Base URI: https://schemas.irods.org/configuration
Administrator Username:     rods
Administrator Password:     Not Shown
-------------------------------------------
Please confirm these settings [yes]:
~~~


### MacOSX

Installation on a MacOSX system requires the use of the --run-in-place build option due to the lack of a system-level package manager.

Building and running iRODS on MacOSX is currently only supported with a locally compiled PostgreSQL.  Once the build.sh script is complete, the PostgreSQL system needs to be configured and made ready to be used by iRODS:

```
 # =-=-=-=-=-=-=-
 # on MacOSX, default postgresql has socket connections managed through /var
 # but its associated helper scripts assume /tmp
 export PGHOST=/tmp

 # =-=-=-=-=-=-=-
 # switch into the new database directory
 cd external/postgresql-9.3.4

 # =-=-=-=-=-=-=-
 # update $PATH to use this local psql
 export PATH=`pwd`/pgsql/bin:$PATH

 # =-=-=-=-=-=-=-
 # initialize the database
 ./pgsql/bin/initdb -D `pwd`/pgsql/data

 # =-=-=-=-=-=-=-
 # turn off standard_conforming_strings (postgresql 9.x)
 sed -i '' 's/#standard_conforming_strings = on/standard_conforming_strings = off/' ./pgsql/data/postgresql.conf

 # =-=-=-=-=-=-=-
 # start the database, with logging
 ./pgsql/bin/pg_ctl -D `pwd`/pgsql/data -l logfile start

 # =-=-=-=-=-=-=-
 # create database, user, and permissions
 ./pgsql/bin/createdb ICAT
 ./pgsql/bin/createuser irods
 ./pgsql/bin/psql ICAT -c "ALTER ROLE irods WITH PASSWORD 'testpassword'"
 ./pgsql/bin/psql ICAT -c "GRANT ALL PRIVILEGES ON DATABASE \"ICAT\" TO irods"

 # =-=-=-=-=-=-=-
 # switch back to top level and run setup_irods_database.sh
 cd ../../
 ./plugins/database/packaging/setup_irods_database.sh
```

# Quickstart

Successful installation will complete and result in a running iRODS server.  The iCommand ``ils`` will list your new iRODS administrator's empty home directory in the iRODS virtual filesystem:

~~~
irods@hostname:~/ $ ils
 /tempZone/home/rods:
~~~

When moving into production, you should cover the following steps as best practice:

## Changing the administrator account password

The default installation of iRODS comes with a single user account 'rods' that is also an admin account ('rodsadmin') with the password 'rods'.  You should change the password before letting anyone else into the system:

~~~
irods@hostname:~/ $ iadmin moduser rods password <newpassword>
~~~

To make sure everything succeeded, you will need to re-authenticate and check the new connection:

~~~
irods@hostname:~/ $ iinit
Enter your current iRODS password:
irods@hostname:~/ $ ils
/tempZone/home/rods:
~~~

If you see an authentication or other error message here, please try again.  The password update only manipulates a single database value, and is independent of other changes to the system.

## Changing the Zone name

The default installation of iRODS comes with a Zone named 'tempZone'.  You probably want to change the Zone name to something more domain-specific:

~~~
irods@hostname:~/ $ iadmin modzone tempZone name <newzonename>
If you modify the local zone name, you and other users will need to
change your irods_environment.json files to use it, you may need to update
server_config.json and, if rules use the zone name, you'll need to update
core.re.  This command will update various tables with the new name
and rename the top-level collection.
Do you really want to modify the local zone name? (enter y or yes to do so):y
OK, performing the local zone rename
~~~

Once the Zone has been renamed, you will need to update two files, one for the 'server', and one for the 'client'.

For the server, you will need to update your `server_config.json` file with the new zone name (a single key/value pair):

~~~
irods@hostname:~/ $ grep zone_name /etc/irods/server_config.json
    "zone_name": "**<newzonename>**",
~~~

For the client, you will need to update your `irods_environment.json` file with the new zone name (three key/value pairs):

~~~
irods@hostname:~/ $ cat .irods/irods_environment.json
{
    "irods_host": "<hostname>",
    "irods_port": 1247,
    "irods_default_resource": "demoResc",
    "irods_home": "/**<newzonename>**/home/rods",
    "irods_cwd": "/**<newzonename>**/home/rods",
    "irods_user_name": "rods",
    "irods_zone_name": "**<newzonename>**",
    "irods_client_server_negotiation": "request_server_negotiation",
    "irods_client_server_policy": "CS_NEG_REFUSE",
    "irods_encryption_key_size": 32,
    "irods_encryption_salt_size": 8,
    "irods_encryption_num_hash_rounds": 16,
    "irods_encryption_algorithm": "AES-256-CBC",
    "irods_default_hash_scheme": "SHA256",
    "irods_match_hash_policy": "compatible",
    "irods_server_control_plane_port": 1248,
    "irods_server_control_plane_key": "TEMPORARY__32byte_ctrl_plane_key",
    "irods_server_control_plane_encryption_num_hash_rounds": 16,
    "irods_server_control_plane_encryption_algorithm": "AES-256-CBC",
    "irods_maximum_size_for_single_buffer_in_megabytes": 32,
    "irods_default_number_of_transfer_threads": 4,
    "irods_transfer_buffer_size_for_parallel_transfer_in_megabytes": 4
}
~~~

Now, the connection should be reset and you should be able to list your empty iRODS collection again:

~~~
irods@hostname:~/ $ iinit
Enter your current iRODS password:
irods@hostname:~/ $ ils
/<newzonename>/home/rods:
~~~

## Changing the zone_key and negotiation_key

iRODS 4.1+ servers use the `zone_key` and `negotiation_key` to mutually authenticate.  These two variables should be changed from their default values in `/etc/irods/server_config.json`:

~~~
"zone_key": "TEMPORARY_zone_key",
"negotiation_key":   "TEMPORARY_32byte_negotiation_key",
~~~

The `zone_key` can be up to 49 alphanumeric characters long and cannot include a hyphen.  The 'negotiation_key' must be exactly 32 alphanumeric bytes long.  These values need to be the same on all servers in the same Zone, or they will not be able to authenticate (see [Server Authentication](federation.md#server-authentication) for more information).

The following error will be logged if a negotiation_key is missing:

~~~
ERROR: client_server_negotiation_for_server - sent negotiation_key is empty
~~~

The following error will be logged when the negotiation_key values do not align and/or are not exactly 32 bytes long:

~~~
ERROR: client-server negotiation_key mismatch
~~~

## Add additional resource(s)

The default installation of iRODS comes with a single resource named 'demoResc' which stores its files in the `/var/lib/irods/iRODS/Vault` directory.  You will want to create additional resources at disk locations of your choosing as the 'demoResc' may not have sufficient disk space available for your intended usage scenarios.  The following command will create a basic 'unixfilesystem' resource at a designated host at the designated full path:

~~~
irods@hostname:~/ $ iadmin mkresc <newrescname> unixfilesystem <fully.qualified.domain.name>:</full/path/to/new/vault>
~~~

Additional information about creating resources can be found with:

~~~
irods@hostname:~/ $ iadmin help mkresc
 mkresc Name Type [Host:Path] [ContextString] (make Resource)
Create (register) a new coordinating or storage resource.

Name is the name of the new resource.
Type is the resource type.
Host is the DNS host name.
Path is the defaultPath for the vault.
ContextString is any contextual information relevant to this resource.
  (semi-colon separated key=value pairs e.g. "a=b;c=d")

A ContextString can be added to a coordinating resource (where there is
no hostname or vault path to be set) by explicitly setting the Host:Path
to an empty string ('').
~~~

Creating new resources does not make them default for any existing or new users.  You will need to make sure that default resources are properly set for newly ingested files.

## Add additional user(s)

The default installation of iRODS comes with a single user 'rods' which is a designated 'rodsadmin' type user account.  You will want to create additional user accounts (of type 'rodsuser') and set their passwords before allowing connections to your new Zone:

~~~
irods@hostname:~/ $ iadmin mkuser <newusername> rodsuser

irods@hostname:~/ $ iadmin lu
rods#tempZone
<newusername>#tempZone

irods@hostname:~/ $ iadmin help mkuser
 mkuser Name[#Zone] Type (make user)
Create a new iRODS user in the ICAT database

Name is the user name to create
Type is the user type (see 'lt user_type' for a list)
Zone is the user's zone (for remote-zone users)

Tip: Use moduser to set a password or other attributes,
     use 'aua' to add a user auth name (GSI DN or Kerberos Principal name)
~~~

It is best to change your Zone name before adding new users as any existing users would need to be informed of the new connection information and changes that would need to be made to their local irods_environment.json files.

