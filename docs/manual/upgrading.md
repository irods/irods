# Upgrading

Upgrading is handled by the host Operating System via the package manager.  Depending on your package manager, your config files will have been preserved with your local changes since the last installation.  Please see [Changing the zone_key and negotiation_key](installation.md#changing-the-zone_key-and-negotiation_key) for information on server-server authentication.

All servers in a Zone must be running the same version of iRODS.  First, upgrade the iCAT server, then upgrade all the Resource servers.

## RPM based systems

~~~
$ (sudo) rpm -U irods-database-plugin-postgres-1.7-opensuse13-x86_64.rpm
$ (sudo) rpm -U irods-icat-TEMPLATE_IRODSVERSION-opensuse13-x86_64.rpm
~~~

The database plugin must be upgraded first when installing RPMs.

## DEB based systems

~~~
$ (sudo) dpkg -i irods-icat-TEMPLATE_IRODSVERSION-ubuntu14-x86_64.deb irods-database-plugin-postgres-1.7-ubuntu14-x86_64.deb
~~~

The database plugin should be upgraded first or at the same time.  Listing them on the same line will allow dpkg to satisfy its dependencies.

## From iRODS 3.3.x

Upgrading from iRODS 3.3.x to iRODS 4.0+ is not supported with an automatic script.  There is no good way to automate setting the new configuration options (resource hierarchies, server_config.json, etc.) based solely on the state of a 3.3.x system.  In addition, with some of the new functionality, a system administrator may choose to implement some existing policies in a different manner with 4.0+.

<span style="color:red">For these reasons, the following manual steps should be carefully studied and understood before beginning the upgrade process.</span>

1. Port any custom development to plugins: Microservices, Resources, Authentication
2. Make a backup of the iCAT database & configuration files: core.re, core.fnm, core.dvm, etc.
3. Declare a Maintenance Window
4. Remove resources from resource groups
5. Remove resource groups (confirm: `iadmin lrg` returns no results)
6. Shutdown 3.3.x server(s)
7. If necessary, start 3.3.x in-place iCAT database ( `irodsctl dbstart` )
8. Install iRODS 4.0+ packages: irods-icat and a database plugin package (e.g. irods-database-plugin-postgres)
9. Patch database with provided upgrade SQL file ( psql ICAT < `packaging/upgrade-3.3.xto4.0.0.sql` )
10. If necessary, migrate 3.3.x in-place iCAT database to the system database installation.  It is recommended to dump and restore your database into the system installation.  This will allow the original database to be uninstalled completely, once the iRODS upgrade is confirmed.
11. Provide a database user 'irods', database password, and owner permissions for that database user to the new system-installed iCAT.
12. Manually update any changes to 'core.re' and 'server_config.json'.  Keep in mind immediate replication rules (`acPostProcForPut`, etc.) may be superceded by your new resource composition.
13. Run `./packaging/setup_irods.sh` (recommended) OR Manually update all 4.0+ configuration files given previous 3.3.x configuration (.irodsEnv, .odbc.ini DSN needs to be set to either 'postgres', 'mysql', or 'oracle').  The automatic ``./packaging/setup_irods.sh`` script will work only with the system-installed database server.
14. Confirm all local at-rest data (any local iRODS Vault paths) has read and write permissions for the new (default) 'irods' unix service account.
15. Start new 4.0+ iCAT server
16. On all resource servers in the same Zone, install and setup 4.0+.  Existing configuration details should be ported as well ('server.config', 'core.re', Vault permissions).
17. Rebuild Resource Hierarchies from previous Resource Group configurations (`iadmin addchildtoresc`) (See [Composable Resources](#composable-resources))
18. Install Custom Plugins (Microservice & Resources)
19. Conformance Testing
20. Sunset 3.3.x server(s)
21. Close Maintenance Window

