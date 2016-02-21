# Upgrading

Upgrading is handled by the host Operating System via the package manager.  Depending on your package manager, your config files will have been preserved with your local changes since the last installation.  Please see [Changing the zone_key and negotiation_key](installation.md#changing-the-zone_key-and-negotiation_key) for information on server-server authentication.

All servers in a Zone must be running the same version of iRODS.  First, upgrade the iCAT server, then upgrade all the Resource servers.

## RPM based systems

~~~
$ (sudo) rpm -U irods-database-plugin-postgres-1.8-opensuse13-x86_64.rpm
$ (sudo) rpm -U irods-icat-TEMPLATE_IRODSVERSION-opensuse13-x86_64.rpm
~~~

The database plugin must be upgraded first when installing RPMs.

## DEB based systems

~~~
$ (sudo) dpkg -i irods-icat-TEMPLATE_IRODSVERSION-ubuntu14-x86_64.deb irods-database-plugin-postgres-1.8-ubuntu14-x86_64.deb
~~~

The database plugin should be upgraded first or at the same time.  Listing them on the same line will allow dpkg to satisfy its depdendencies.

## Run-in-Place systems

Run-in-Place has been made available for testing and backwards compatibility.  The lack of managed update scripts, coupled with a growing array of possible plugin combinations, will make sustaining a '--run-in-place' installation much more challenging.

It is possible that in the 5.0 timeframe, with the plans for a proper plugin registry, managing a '--run-in-place' installation can be handled more gracefully.

The best way to manage a '--run-in-place' installation at this time is to checkout a newer release, rebuild, and then re-run the setup script:

~~~
$ cd irods
$ git checkout TEMPLATE_IRODSVERSION
$ ./packaging/build.sh --run-in-place icat postgres
$ ./plugins/database/packaging/setup_irods_database.sh
~~~

## From iRODS 3.3.x

Migrating from iRODS 3.3.x (run-in-place) to iRODS 4.0+ is not supported with an automatic script.  There is no good way to automate setting the new configuration options (resource hierarchies, server_config.json, etc.) based solely on the state of a 3.3.x system.  In addition, with some of the new functionality, a system administrator may choose to implement some existing policies in a different manner with 4.0+.

<span style="color:red">For these reasons, the following manual steps should be carefully studied and understood before beginning the migration process.</span>

1. Port any existing custom development to plugins: Microservices, Resources, Authentication
2. Make a backup of the iCAT database, and all iRODS configuration files: core.re, core.fnm, core.dvm, server.config, custom rulefiles, server's .irodsEnv
3. Declare a Maintenance Window
4. Remove resources from resource groups
5. Remove resource groups (confirm: `iadmin lrg` returns no results)
6. Shutdown 3.3.x server(s)
7. Make sure existing 3.3.x iCAT database is available (either `irodsctl dbstart`, or externally managed database)
8. Install iRODS 4.0+ packages: irods-icat and a database plugin package (e.g. irods-database-plugin-postgres)
9. Patch existing database with provided upgrade SQL file (psql ICAT < `packaging/upgrade-3.3.xto4.0.0.sql`)
10. If necessary, migrate 3.3.x in-place iCAT database to the system database installation.  It is recommended to dump and restore your database into the system installation.  This will allow the original database to be uninstalled completely, once the iRODS upgrade is confirmed.  If you were already using an externally managed database and want to continue with that arrangement, this step is not necessary.
11. For a new local system database installation, provide a database user 'irods', database password, and owner permissions for that database user to the new iCAT.  If you are using an externally managed database, this step is not necessary as these values should already exist.
12. Manually update any changes to 'core.re' and 'server_config.json'.  Keep in mind immediate replication rules (`acPostProcForPut`, etc.) may be superceded by your new resource composition.  Scripting this step may be possible using the `./packaging/update_json.py` script.
13. Run `./packaging/setup_irods.sh` (recommended) OR Manually update all 4.0+ configuration files given previous 3.3.x configuration (.irodsEnv, .odbc.ini DSN needs to be set to either 'postgres', 'mysql', or 'oracle').  The automatic ``./packaging/setup_irods.sh`` script will work only when targeting the system-installed database server or an externally managed database server (it will not know about the legacy 3.3.x location).
14. Confirm all local at-rest data (any local iRODS Vault paths) have read and write permissions for the new (default) 'irods' unix service account.
15. Start new 4.0+ iCAT server (`irodsctl start`)
16. On all resource servers in the same Zone, install and setup 4.0+.  Existing configuration details should be ported as well ('server.config', 'core.re', Vault permissions).
17. Rebuild Resource Hierarchies from previous Resource Group configurations (`iadmin addchildtoresc`) (See [Composable Resources](architecture.md#composable-resources))
18. Install any custom plugins (Microservice, Resources, Authentication)
19. Perform your conformance testing
20. Sunset 3.3.x server(s)
21. Close your Maintenance Window
22. Share with your users any relevant changes to their connection credentials (possibly nothing to do here).

!!! Note
    Migrating from in-place 3.3.x to a ['--run-in-place' production installation of 4.0+](#run-in-place-systems) is not recommended.
