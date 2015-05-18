# iRODS Administration Guide

## Overview

This manual provides standalone documentation for [iRODS](https://irods.org) as packaged by the [Renaissance Computing Institute (RENCI)](http://www.renci.org) under the aegis of the [iRODS Consortium](https://irods.org/consortium).

Full definitions for terms discussed in this manual are available in the [Glossary](../User_Guide/glossary.md).

## Download

iRODS is released in both binary package format and with full source code.

### Binaries

RPM and DEB formats are available for both iCAT-enabled servers and resource-only servers.  There are variations available for combinations of platform and operating system.

More combinations will be made available as our testing matrix continues to mature and increase in scope.

The latest files can be downloaded from [https://irods.org/download](https://irods.org/download).

### Open Source

Repositories, issue trackers, and source code are available on GitHub.

 [https://github.com/irods](https://github.com/irods)

## Upgrading

Upgrading is handled by the host Operating System via the package manager.  Depending on your package manager, your config files will have been preserved with your local changes since the last installation.  Please see [Changing the zone_key and negotiation_key](#changing-the-zone_key-and-negotiation_key) for information on server-server authentication.

### RPM based systems

~~~
$ (sudo) rpm -U irods-icat-TEMPLATE_IRODSVERSION-64bit-suse.rpm
~~~

### DEB based systems

~~~
$ (sudo) dpkg -i irods-icat-TEMPLATE_IRODSVERSION-64bit.deb
~~~

### From iRODS 3.3.x

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

## Backing Up

Backing up iRODS involves: The data, the iRODS system and configuration files, and the iCAT database itself.

Configuration and maintenance of this type of backup system is out of scope for this document, but is included here as an indication of best practice.

1. The data itself can be handled by the iRODS system through replication and should not require any specific backup efforts worth noting here.
2. The state of a zone (current policy and configuration settings) can be gathered by a 'rodsadmin' by running `izonereport`.  The resulting JSON file can be stored into iRODS or saved somewhere else.  When run on a regular schedule, the `izonereport` can gather all the necessary configuration information to help you reconstruct your iRODS setup during disaster recovery.
3. The iCAT database itself can be backed up in a variety of ways.  A PostgreSQL database is contained on the local filesystem as a data/ directory and can be copied like any other set of files.  This is the most basic means to have backup copies.  However, this will have stale information almost immediately.  To cut into this problem of staleness, PostgreSQL 8.4+ includes a feature called ["Record-based Log Shipping"](http://www.postgresql.org/docs/8.4/static/warm-standby.html#WARM-STANDBY-RECORD).  This consists of sending a full transaction log to another copy of PostgreSQL where it could be "re-played".  This would bring the copy up to date with the originating server.  Log shipping would generally be handled with a cronjob.  A faster, seamless version of log shipping called ["Streaming Replication"](http://www.postgresql.org/docs/9.0/static/warm-standby.html#STREAMING-REPLICATION) was included in PostgreSQL 9.0+ and can keep two PostgreSQL servers in sync with sub-second delay.

## Architecture

iRODS 4.0+ represents a major effort to analyze, harden, and package iRODS for sustainability, modularization, security, and testability.  This has led to a fairly significant refactorization of much of the underlying codebase.  The following descriptions are included to help explain the architecture of iRODS.

The core is designed to be as immutable as possible and serve as a bus for handling the internal logic of the business of iRODS (data storage, policy enforcement, etc.).  Seven or eight major interfaces will be exposed by the core and will allow extensibility and separation of functionality into plugins.  A few plugins are included by default in iRODS to provide a set of base functionality.

The planned plugin interfaces and their status are listed here:

| Plugin Interface               | Status     |  Since   |
| ------------------------------ | ---------- | -------- |
| Pluggable Microservices        | Complete   |  3.0b2   |
| Composable Resources           | Complete   |  3.0b3   |
| Pluggable Authentication       | Complete   |  3.0.1b1 |
| Pluggable Network              | Complete   |  3.0.1b1 |
| Pluggable Database             | Complete   |  4.0.0b1 |
| Pluggable RPC API              | Complete   |  4.0.0b2 |
| Pluggable Rule Engine          | Requested  |          |
| Pluggable First Class Objects  | Requested  |          |


### Dynamic Policy Enforcement Points

iRODS 4.0+ has introduced the capability for dynamic policy enforcement points (PEP).  For every operation that is called, two policy enforcement points are constructed (both a pre and post variety), and if it has been defined in core.re or any other loaded rulebase file they will be executed by the rule engine.

The PEP will be constructed of the form "pep_PLUGINOPERATION_pre" and "pep_PLUGINOPERATION_post".

For example, for "resource_create", the two PEPs that are dynamically evaluated are pep_resource_create_pre(\*OUT) and pep_resource_create_post(\*OUT).  If either or both have been defined in a loaded rulebase file (core.re), they will be executed as appropriate.

The flow of information from the pre PEP to the plugin operation to the post PEP works as follows:

- pep_PLUGINOPERATION_pre(\*OUT) - Should produce an \*OUT variable that will be passed to the calling plugin operation
- PLUGINOPERATION - Will receive any \*OUT defined by pep_PLUGINOPERATION_pre(\*OUT) above and will pass its own \*OUT variable to pep_PLUGINOPERATION_post()
- pep_PLUGINOPERATION_post() - Will receive any \*OUT from PLUGINOPERATION.  If the PLUGINOPERATION itself failed, the \*OUT variable will be populated with the string "OPERATION_FAILED".



#### Available Plugin Operations

The following operations are available for dynamic PEP evaluation.  At this time, only very few operations themselves consider the output (\*OUT) of its associated pre PEP.

<table border="1">
<tr><th>Plugin Type</th><th>Plugin Operation</th></tr>
<tr>
<td>Resource</td>
<td>resource_create<br />
resource_open<br />
resource_read<br />
resource_write<br />
resource_stagetocache<br />
resource_synctoarch<br />
resource_registered<br />
resource_unregistered<br />
resource_modified<br />
resource_resolve_hierarchy<br />
resource_rebalance
</td>
</tr>
<tr>
<td>Authentication</td>
<td>auth_client_start<br />
auth_agent_start<br />
auth_establish_context<br />
auth_agent_client_request<br />
auth_agent_auth_request<br />
auth_agent_client_response<br />
auth_agent_auth_response<br />
auth_agent_auth_verify
</td>
</tr>
<tr>
<td>Network</td>
<td>network_client_start<br />
network_client_stop<br />
network_agent_start<br />
network_agent_stop<br />
network_read_header<br />
network_read_body<br />
network_write_header<br />
network_write_body
</td>
</tr>
</table>

#### Available Values within Dynamic PEPs

The following Key-Value Pairs are made available within the running context of each dynamic policy enforcement point (PEP) based both on the plugin type as well as the first class object of interest.  They are available via the rule engine in the form of `$KVPairs.VARIABLE_NAME` and are originally defined in `iRODS/lib/core/include/rodsKeyWdDef.h`.


<table border="1">
<tr><th>Plugin Type</th><th>First Class Object</th><th>Variable Name</th></tr>
<tr>
<td rowspan="4">Resource</td><td>Data Object</td>
<td>
physical_path<br />
mode_kw<br />
flags_kw<br />
resc_hier
</td>
</tr>
<tr><td>File Object</td>
<td>logical_path<br />
file_descriptor<br />
l1_desc_idx<br />
file_size<br />
repl_requested<br />
in_pdmo
</td>
</tr>
<tr><td>Structured Object</td>
<td>host_addr<br />
zone_name<br />
port_num<br />
sub_file_path<br />
offset<br />
dataType<br />
oprType
</td>
</tr>
<tr><td>Special Collection</td>
<td>spec_coll_class<br />
spec_coll_type<br />
spec_coll_obj_path<br />
spec_coll_resource<br />
spec_coll_resc_hier<br />
spec_coll_phy_path<br />
spec_coll_cache_dir<br />
spec_coll_cache_dirty<br />
spec_coll_repl_num
</td>
</tr>
<tr><td>Authentication</td>
<td>Native Password<br />
OS Auth<br />
PAM
</td>
<td>zone_name<br />
user_name<br />
digest
</td>
</tr>
<tr><td rowspan="2">Network</td>
<td>TCP</td>
<td>tcp_socket_handle</td>
</tr>
<tr><td>SSL</td>
<td>ssl_host<br />
ssl_shared_secret<br />
ssl_key_size<br />
ssl_salt_size<br />
ssl_num_hash_rounds<br />
ssl_algorithm
</td>
</tr>
</table>


For example, within a PEP, you could reference $KVPairs.file_size and get the size of the file currently in context.  Likewise, $KVPairs.ssl_host would provide the current hostname involved in an SSL connection.

Also, $pluginInstanceName is an additional available session variable that gives the instance name of the plugin from which the call is made.

For example, when running `iput -R myOtherResc newfile.txt`, a `fileCreate()` operation is called on "myOtherResc".  This delegates the call to the myOtherResc plugin instance which is a "resource_create" operation.  When the pep_resource_create_pre() rule is evaluated, the value of $pluginInstanceName will be "myOtherResc".  This allows rule authors to make decisions at a per-resource basis for this type of operation.

## Pluggable Microservices

iRODS is in the process of being modularized whereby existing iRODS 3.x functionality will be replaced and provided by small, interoperable plugins.  The first plugin functionality to be completed was pluggable microservices.  Pluggable microservices allow users to add new microservices to an existing iRODS server without recompiling the server or even restarting any running processes.  A microservice plugin contains a single compiled microservice shared object file to be found by the server.  Development examples can be found in the source tree under [examples/microservices](https://github.com/irods/irods/tree/master/examples/microservices).

A separate development package, irods-dev, available at [https://irods.org/download](https://irods.org/download), contains the necessary header files to write your own microservice plugins (as well as any other type of iRODS plugin).  Additional information can be found in the [Microservice Developers Tutorial](https://github.com/irods/irods/blob/master/examples/microservices/microservice_tutorial.rst).

## Composable Resources

The second area of modularity to be added to iRODS 4.0+ consists of composable resources.  Composable resources replace the concept of resource groups from iRODS 3.x.  There are no resource groups in iRODS 4.0+.

### Tree Metaphor

In computer science, a tree is a data structure with a hierarchical representation of linked nodes. These nodes can be named based on where they are in the hierarchy. The node at the top of a tree is the root node. Parent nodes and child nodes are on opposite ends of a connecting link, or edge. Leaf nodes are at the bottom of the tree, and any node that is not a leaf node is a branch node. These positional descriptors are helpful when describing the structure of a tree. Composable resources are best represented using this tree metaphor.

An iRODS composite resource is a tree with one 'root' node.  Nodes that are at the bottom of the tree are 'leaf' nodes.  Nodes that are not leaf nodes are 'branch' nodes and have one or more 'child' nodes.  A child node can have one and only one 'parent' node.

The terms root, leaf, branch, child, and parent represent locations and relationships within the structure of a particular tree.  To represent the functionality of a particular resources within a particular tree, the terms 'coordinating' and 'storage' are used in iRODS.  Coordinating resources coordinate the flow of data to and from other resources.  Storage resources are typically 'leaf' nodes and handle the direct reading and writing of data through a POSIX-like interface.

Any resource node can be a coordinating resource and/or a storage resource.  However, for clarity and reuse, it is generally best practice to separate the two so that a particular resource node is either a coordinating resource or a storage resource.

This powerful tree metaphor is best illustrated with an actual example.  You can now use `ilsresc --tree` to visualize the tree structure of a Zone.

![ilsresc --tree](treeview.png "alt title")

### Virtualization

In iRODS, files are stored as Data Objects on disk and have an associated physical path as well as a virtual path within the iRODS file system. iRODS collections, however, only exist in the iCAT database and do not have an associated physical path (allowing them to exist across all resources, virtually).

Composable resources, both coordinating and storage, introduce the same dichotomy between the virtual and physical.  A coordinating resource has built-in logic that defines how it determines, or coordinates, the flow of data to and from its children. Coordinating resources exist solely in the iCAT and exist virtually across all iRODS servers in a particular Zone. A storage resource has a Vault (physical) path and knows how to speak to a specific type of storage medium (disk, tape, etc.). The encapsulation of resources into a plugin architecture allows iRODS to have a consistent interface to all resources, whether they represent coordination or storage.

This virtualization enables the coordinating resources to manage both the placement and the retrieval of Data Objects independent from the types of resources that are connected as children resources. When iRODS tries to retrieve data, each child resource will "vote", indicating whether it can provide the requested data.  Coordinating resources will then decide which particular storage resource (e.g. physical location) the read should come from. The specific manner of this vote is specific to the logic of the coordinating resource.  A coordinating resource may lean toward a particular vote based on the type of optimization it deems best. For instance, a coordinating resource could decide between child votes by opting for the child that will reduce the number of requests made against each storage resource within a particular time frame or opting for the child that reduces latency in expected data retrieval times. We expect a wide variety of useful optimizations to be developed by the community.

An intended side effect of the tree metaphor and the virtualization of coordinating resources is the deprecation of the concept of a resource group. Resource groups in iRODS 3.x could not be put into other resource groups. A specific limiting example is a compound resource that, by definition, was a group and could not be placed into another group.  This significantly limited its functionality as a management tool. Groups in iRODS now only refer to user groups.

Read more about [Composable Resources](https://irods.org/2013/02/e-irods-composable-resources/):

- [Paper (279kB, PDF)](https://irods.org/wp-content/uploads/2013/02/eirods-composable-resources.pdf)
- [Slides (321kB, PDF)](https://irods.org/wp-content/uploads/2013/02/eirods-cr-slides.pdf)
- [Poster (6.4MB, PDF)](https://irods.org/wp-content/uploads/2013/02/eirods-composable-resources-poster.pdf)

### Coordinating Resources

Coordinating resources contain the flow control logic which determines both how its child resources will be allocated copies of data as well as which copy is returned when a Data Object is requested.  There are several types of coordinating resources: compound, random, replication, round robin, passthru, and some additional types that are expected in the future.  Each is discussed in more detail below.

#### Compound

The compound resource is a continuation of the legacy compound resource type from iRODS 3.x.

A compound resource has two and only two children.  One must be designated as the 'cache' resource and the other as the 'archive' resource.  This designation is made in the "context string" of the `addchildtoresc` command.

An Example:

~~~
irods@hostname:~/ $ iadmin addchildtoresc parentResc newChildResc1 cache
irods@hostname:~/ $ iadmin addchildtoresc parentResc newChildResc2 archive
~~~

Putting files into the compound resource will first create a replica on the cache resource and then create a second replica on the archive resource.

Getting files from the compound resource will behave in a similar way as iRODS 3.x.  By default, the replica from the cache resource will always be returned.  If the cache resource does not have a copy, then a replica is created on the cache resource before being returned.

This compound resource staging policy can be controlled with the policy key-value pair whose keyword is "compound_resource_cache_refresh_policy" and whose values are either "when_necessary" (default), or "always".

From the example near the bottom of the core.re rulebase:

~~~
# =-=-=-=-=-=-=-
# policy controlling when a dataObject is staged to cache from archive in a compound coordinating resource
#  - the default is to stage when cache is not present ("when_necessary")
# =-=-=-=-=-=-=-
# pep_resource_resolve_hierarchy_pre( *OUT ){*OUT="compound_resource_cache_refresh_policy=when_necessary";}  # default
# pep_resource_resolve_hierarchy_pre( *OUT ){*OUT="compound_resource_cache_refresh_policy=always";}
~~~

Replicas within a compound resource can be trimmed.  There is no rebalance activity defined for a compound resource.  When the cache fills up, the administrator will need to take action as they see fit.  This may include physically moving files to other resources, commissioning new storage, or marking certain resources "down" in the iCAT.

The "--purgec" option for `iput`, `iget`, and `irepl` is honored and will always purge the first replica (usually with replica number 0) for that Data Object (regardless of whether it is held within this compound resource).  This is not an optimal use of the compound resource as the behavior will become somewhat nondeterministic with complex resource compositions.

#### Deferred

The deferred resource is designed to be as simple as possible.  A deferred resource can have one or more children.

A deferred resource provides no implicit data management policy.  It defers to its children with respect to routing both puts and gets.  However they vote, the deferred node decides.

#### Load Balanced

The load balanced resource provides equivalent functionality as the "doLoad" option for the `msiSetRescSortScheme` microservice.  This resource plugin will query the `r_server_load_digest` table from the iCAT and select the appropriate child resource based on the load values returned from the table.

The `r_server_load_digest` table is part of the Resource Monitoring System and has been incorporated into iRODS 4.x.  The r_server_load_digest table must be populated with load data for this plugin to function properly.

The load balanced resource has an effect on writes only (it has no effect on reads).

#### Random

The random resource provides logic to put a file onto one of its children on a random basis.  A random resource can have one or more children.

If the selected target child resource of a put operation is currently marked "down" in the iCAT, the random resource will move on to another random child and try again.  The random resource will try each of its children, and if still not succeeding, throw an error.

#### Replication

The replication resource provides logic to automatically manage replicas to all its children.

[Rebalancing](#rebalancing) of the replication node is made available via the "rebalance" subcommand of `iadmin`.  For the replication resource, all Data Objects on all children will be replicated to all other children.  The amount of work done in each iteration as the looping mechanism completes is controlled with the session variable `replication_rebalance_limit`.  The default value is set at 500 Data Objects per loop.

Getting files from the replication resource will show a preference for locality.  If the client is connected to one of the child resource servers, then that replica of the file will be returned, minimizing network traffic.

#### Round Robin

The round robin resource provides logic to put a file onto one of its children on a rotating basis.  A round robin resource can have one or more children.

If the selected target child resource of a put operation is currently marked "down" in the iCAT, the round robin resource will move onto the next child and try again.  If all the children are down, then the round robin resource will throw an error.

#### Passthru

The passthru resource was originally designed as a testing mechanism to exercise the new composable resource hierarchies.

A passthru resource can have one and only one child.

#### Expected

A few other coordinating resource types have been brainstormed but are not functional at this time:

 - Storage Balanced (%-full) (expected)
 - Storage Balanced (bytes) (expected)
 - Tiered (expected)

### Storage Resources

Storage resources represent storage interfaces and include the file driver information to talk with different types of storage.

#### Unix File System

The unix file system storage resource is the default resource type that can communicate with a device through the standard POSIX interface.

#### Structured File Type (tar, zip, gzip, bzip)

The structured file type storage resource is used to interface with files that have a known format.  By default these are used "under the covers" and are not expected to be used directly by users (or administrators).

These are used mainly for mounted collections.

#### Amazon S3 (Archive)

The Amazon S3 archive storage resource is used to interface with an S3 bucket.  It is expected to be used as the archive child of a compound resource composition.  The credentials are stored in a file which is referenced by the context string.  Read more at: [https://github.com/irods/irods_resource_plugin_s3](https://github.com/irods/irods_resource_plugin_s3)

#### DDN WOS (Archive)

The DataDirect Networks (DDN) WOS archive storage resource is used to interface with a Web Object Scalar (WOS) Appliance.  It is expected to be used as the archive child of a compound resource composition.  It currently references a single WOS endpoint and WOS policy in the context string.  Read more at: [https://github.com/irods/irods_resource_plugin_wos](https://github.com/irods/irods_resource_plugin_wos)

#### Non-Blocking

The non-blocking storage resource behaves exactly like the standard unix file system storage resource except that the "read" and "write" operations do not block (they return immediately while the read and write happen independently).

#### Mock Archive

The mock archive storage resource was created mainly for testing purposes to emulate the behavior of object stores (e.g. WOS).  It creates a hash of the file path as the physical name of the Data Object.

#### Universal Mass Storage Service

The univMSS storage resource delegates stage_to_cache and sync_to_arch operations to an external script which is located in the iRODS/server/bin/cmd directory.  It currently writes to the Vault path of that resource instance, treating it as a unix file system.

When creating a "univmss" resource, the context string provides the location of the Universal MSS script.

Example:

~~~
irods@hostname:~/ $ iadmin mkresc myArchiveResc univmss HOSTNAME:/full/path/to/Vault univMSSInterface.sh
~~~

#### Expected

A few other storage resource types are under development and will be released as additional separate plugins:

 - ERDDAP (expected)
 - HDFS (expected)
 - HPSS (expected)
 - Pydap (expected)
 - TDS (expected)

### Managing Child Resources

There are two new ``iadmin`` subcommands introduced with this feature.

`addchildtoresc`:

~~~
irods@hostname:~/ $ iadmin h addchildtoresc
 addchildtoresc Parent Child [ContextString] (add child to resource)
Add a child resource to a parent resource.  This creates an 'edge'
between two nodes in a resource tree.

Parent is the name of the parent resource.
Child is the name of the child resource.
ContextString is any relevant information that the parent may need in order
  to manage the child.
~~~

`rmchildfromresc`:

~~~
irods@hostname:~/ $ iadmin h rmchildfromresc
 rmchildfromresc Parent Child (remove child from resource)
Remove a child resource from a parent resource.  This removes an 'edge'
between two nodes in a resource tree.

Parent is the name of the parent resource.
Child is the name of the child resource.
~~~

### Example Usage

Creating a composite resource consists of creating the individual nodes of the desired tree structure and then connecting the parent and children nodes.

#### Example 1

![example1 tree](example1-tree.jpg)

**Example 1:** Replicates Data Objects to three locations

A replicating coordinating resource with three unix file system storage resources as children would be composed with seven (7) iadmin commands:

~~~
irods@hostname:~/ $ iadmin mkresc example1 replication
irods@hostname:~/ $ iadmin mkresc repl_resc1 unixfilesystem renci.example.org:/Vault
irods@hostname:~/ $ iadmin mkresc repl_resc2 unixfilesystem sanger.example.org:/Vault
irods@hostname:~/ $ iadmin mkresc repl_resc3 unixfilesystem eudat.example.org:/Vault
irods@hostname:~/ $ iadmin addchildtoresc example1 repl_resc1
irods@hostname:~/ $ iadmin addchildtoresc example1 repl_resc2
irods@hostname:~/ $ iadmin addchildtoresc example1 repl_resc3
~~~

### Rebalancing

A new subcommand for iadmin allows an administrator to rebalance a coordinating resource.  The coordinating resource can be the root of a tree, or anywhere in the middle of a tree.  The rebalance operation will rebalance for all decendents.  For example, the iadmin command `iadmin modresc myReplResc rebalance` would fire the rebalance operation for the replication resource instance named myReplResc.  Any Data Objects on myReplResc that did not exist on all its children would be replicated as expected.

For other coordinating resource types, rebalance can be defined as appropriate.  For coordinating resources with no concept of "balanced", the rebalance operation is a "no op" and performs no work.

## Pluggable Authentication

The authentication methods are now contained in plugins.  By default, similar to iRODS 3.3 and prior, iRODS comes with native iRODS challenge/response (password) enabled.  However, enabling an additional authentication mechanism is as simple as adding a file to the proper directory.  The server does not need to be restarted.

Available authentication mechanisms include:

- Native iRODS password
- OSAuth
- GSI (Grid Security Infrastructure)
- PAM (Pluggable Authentication Module)
- Kerberos
- LDAP (via PAM)

## Pluggable Network

iRODS now ships with both TCP and SSL network plugins enabled.  The SSL mechanism is provided via OpenSSL and wraps the activity from the TCP plugin.

The SSL parameters are tunable via the following `irods_environment.json` variables:

~~~
"irods_client_server_negotiation": "request_server_negotiation",
"irods_client_server_policy": "CS_NEG_REQUIRE",
"irods_encryption_key_size": 32,
"irods_encryption_salt_size": 8,
"irods_encryption_num_hash_rounds": 16,
"irods_encryption_algorithm": "AES-256-CBC",
~~~

The only valid value for 'irods_client_server_negotiation' at this time is 'request_server_negotiation'.  Anything else will not begin the negotiation stage and default to using a TCP connection.

The possible values for irodsClientServerPolicy include:

- CS_NEG_REQUIRE: This side of the connection requires an SSL connection
- CS_NEG_DONT_CARE: This side of the connection will connect either with or without SSL
- CS_NEG_REFUSE: (default) This side of the connection refuses to connect via SSL

In order for a connection to be made, the client and server have to agree on the type of connection they will share.  When both sides choose ``CS_NEG_DONT_CARE``, iRODS shows an affinity for security by connecting via SSL.  Additionally, it is important to note that all servers in an iRODS Zone are required to share the same SSL credentials (certificates, keys, etc.).  Maintaining per-route certificates is not supported at this time.

The remaining parameters are standard SSL parameters and made available through the EVP library included with OpenSSL.  You can read more about these remaining parameters at [https://www.openssl.org/docs/crypto/evp.html](https://www.openssl.org/docs/crypto/evp.html).

## Pluggable Database

The iRODS metadata catalog is now installed and managed by separate plugins.  The TEMPLATE_IRODSVERSION release has PostgreSQL, MySQL, and Oracle database plugins available and tested.  MySQL is not available on CentOS 5, as the required set of `lib_mysqludf_preg` functions are not currently available on that OS.

The particular type of database is encoded in `/etc/irods/database_config.json` with the following directive:

~~~
"catalog_database_type" : "postgres",
~~~

This is populated by the `setup_irods_database.sh` script on configuration.

The iRODS 3.x icatHighLevelRoutines are, in effect, the API calls for the database plugins.  No changes should be needed to any calls to the icatHighLevelRoutines.

To implement a new database plugin, a developer will need to provide the existing 84 SQL calls (in icatHighLevelRoutines) and an implementation of GenQuery.

### Installing lib_mysqludf_preg

To use the iRODS MySQL database plugin, the MySQL server must have the [lib_mysqludf_preg functions](https://github.com/mysqludf/lib_mysqludf_preg) installed and available to iRODS. iRODS installation (`setup_irods.sh`) will fail if these functions are not installed on the database.

The steps for installing `lib_mysqludf_preg` on Ubuntu 14.04 are:

~~~
# Get Dependencies
sudo apt-get install mysql-server mysql-client libmysqlclient-dev libpcre3-dev automake libtool

# Build and Install
cd lib_mysqludf_preg
autoreconf --force --install
./configure
make
sudo make install
sudo make MYSQL="mysql -p" installdb
~~~

To test the installation, execute the following command::

~~~
$ mysql --user=$MYSQL_IRODS_ACCOUNT --password=$MYSQL_IRODS_PASSWORD -e "select PREG_REPLACE('/failed/i', 'succeeded', 'Call to PREG_REPLACE() failed.')"
~~~

which should display the following output:

~~~
+--------------------------------------------------------------------------+
| PREG_REPLACE('/failed/i', 'succeeded', 'Call to PREG_REPLACE() failed.') |
+--------------------------------------------------------------------------+
| Call to PREG_REPLACE() succeeded.                                        |
+--------------------------------------------------------------------------+
~~~

## Pluggable RPC API

The iRODS API has traditionally been a hard-coded table of values and names.  With the pluggable RPC API now available, a plugin can provide new API calls.

At runtime, if a reqested API number is not already in the table, it is dynamically loaded from `plugins/api` and executed.  As it is a dynamic system, there is the potential for collisions between existing API numbers and any new dynamically loaded API numbers.  It is considered best practice to use a dynamic API number above 10000 to ensure no collisions with the existing static API calls.

API plugins self-describe their IN and OUT packing instructions (examples coming soon).  These packing instructions are loaded into the table at runtime along with the API name, number, and the operation implementation being described.




## Users & Permissions

Users and permissions in iRODS are inspired by, but slightly different from, traditional Unix filesystem permissions.  Access to Data Objects and Collections can be modified using the `ichmod` iCommand.

Additionally, permissions can be managed via user groups in iRODS.  A user can belong to more than one group at a time.  The owner of a Data Object has full control of the file and can grant and remove access to other users and groups.  The owner of a Data Object can also give ownership rights to other users, who in turn can grant or revoke access to users.

Inheritance is a collection-specific setting that determines the permission settings for new Data Objects and sub-Collections.  Data Objects created within Collections with Inheritance set to Disabled do not inherit the parent Collection's permissions.  By default, iRODS has Inheritance set to Disabled.  More can be read from the help provided by `ichmod h`.

Inheritance is especially useful when working with shared projects such as a public Collection to which all users should have read access. With Inheritance set to Enabled, any sub-Collections created under the public Collection will inherit the properties of the public Collection.  Therefore, a user with read access to the public Collection will also have read access to all Data Objects and Collections created in the public Collection.

## Rule Engine

The Rule Engine, which keeps track of state and interprets both system-defined rules and user-defined rules, is a critical component of the iRODS system.  Rules are definitions of actions that are to be performed by the server.  These actions are defined in terms of microservices and other actions.  The iRODS built-in Rule Engine interprets the rules and calls the appropriate microservices.

### File Locking

A race condition occurs when two processes simultaneously try to change the same data. The outcome of a race condition is unpredictable since both threads are "racing" to update the data.  To allow iRODS users to control such events, the iCommands `iput`, `iget`, and `irepl` each have both --wlock and --rlock options to lock the Data Objects during these operations.  An irodsServer thread then purges unused locked files every 2 hours.

### Delay execution

Rules can be run in two modes - immediate execution or delayed execution.  Most of the actions and microservices executed by the rule engine are executed immediately, however, some actions are better suited to be placed in a queue and executed later.  The actions and microservices which are to be executed in delay mode can be queued with the `delay` microservice.  Typically, delayed actions and microservices are resource-heavy, time-intensive processes, better suited to being carried out without having the user wait for their completion.  These delayed processes can also be used for cleanup and general maintenance of the iRODS system, like the cron in UNIX.

Monitoring the delayed queue is important once your workflows and maintenance scripts depends on the health of the system. The delayed queue can be managed with the following three iCommands:

1. iqdel    - remove a delayed rule (owned by you) from the queue.
2. iqmod    - modify certain values in existing delayed rules (owned by you).
3. iqstat   - show the queue status of delayed rules.

### Additional Information

More information is available in the standalone document named [Rule Language](../User_Guide/Rule_Language.md)

<!--
..
.. ---------------
.. Delay Execution
.. ---------------
.. - how
.. - what
.. - when
.. - where
.. - why
.. - errors
.. - queue management
.. - file locking
..
.. ----------
.. Monitoring
.. ----------
.. - nagios plugins (Jean-Yves)
.. - other
.. - Failover checking
..
-->

## Authentication

By default, iRODS uses a secure password system for user authentication.  The user passwords are scrambled and stored in the iCAT database.  Additionally, iRODS supports user authentication via PAM (Pluggable Authentication Modules), which can be configured to support many things, including the LDAP authentication system.  PAM and SSL have been configured 'available' out of the box with iRODS, but there is still some setup required to configure an installation to communicate with your local external authentication server of choice.

The iRODS administrator can 'force' a particular auth scheme for a rodsuser by 'blanking' the native password for the rodsuser.  There is currently no way to signal to a particular login attempt that it is using an incorrect scheme ([GitHub Issue #2005](https://github.com/irods/irods/issues/2005)).

### GSI

Grid Security Infrastructure (GSI) setup in iRODS 4.0+ has been greatly simplified.  The functionality itself is provided by the [GSI auth plugin](https://github.com/irods/irods_auth_plugin_gsi).

#### GSI Configuration

Configuration of GSI is out of scope for this document, but consists of the following three main steps:

1. Install GSI (most easily done via package manager)
2. Confirm the (default) irods service account has a certificate in good standing (signed)
3. Confirm the local system account for client "newuser" account has a certificate in good standing (signed)

#### iRODS Configuration

Configuring iRODS to communicate via GSI requires a few simple steps.

First, if GSI is being configured for a new user, it must be created:

~~~
iadmin mkuser newuser rodsuser
~~~

Then that user must be configured so its Distiguished Name (DN) matches its certificate:

~~~
iadmin aua newuser '/DC=org/DC=example/O=Example/OU=People/CN=New User/CN=UID:drexample'
~~~

!!! Note
    The comma characters (,) in the Distiguished Name (DN) must be replaced with forward slash characters (/).

On the client side, the user's 'irods_auth_scheme' must be set to 'GSI'.  This can be done via environment variable:

~~~
irods@hostname:~/ $ irods_auth_scheme=GSI
irods@hostname:~/ $ export irods_auth_scheme
~~~

Or, preferably, in the user's `irods_environment.json` file:

~~~
"irods_auth_scheme": "GSI",
~~~

Then, to have a temporary proxy certificate issued and authenticate:

~~~
grid-proxy-init
~~~

This will prompt for the user's GSI password.  If the user is successfully authenticated, temporary certificates are issued and setup in the user's environment.  The certificates are good, by default, for 24 hours.

In addition, if users want to authenticate the server, they can set 'irods_gsi_server_dn' in their user environment. This will cause the system to do mutual authentication instead of just authenticating the client user to the server.

#### Limitations

The iRODS administrator will see two limitations when using GSI authentication:

1. The 'client_user_name' environment variable will fail (the admin cannot alias as another user)
2. The `iadmin moduser password` will fail (cannot update the user's password)

The workaround is to use iRODS native password authentication when using these.

`ipasswd` for rodsusers will also fail, but it is not an issue as it would be trying to update their (unused) iRODS native password.  They should not be updating their GSI passwords via iCommands.

### Kerberos

Kerberos setup in iRODS 4.0+ has been greatly simplified.  The functionality itself is provided by the [Kerberos auth plugin](https://github.com/irods/irods_auth_plugin_kerberos).

#### Kerberos Configuration

Configuration of Kerberos is out of scope for this document, but consists of the following four main steps:

1. Set up Kerberos (Key Distribution Center (KDC) and Kerberos Admin Server)
2. Confirm the (default) irods service account has a service principal in KDC (with the hostname of the rodsServer) (e.g. irodsserver/serverhost.example.org@EXAMPLE.ORG)
3. Confirm the local system account for client "newuser" account has principal in KDC (e.g. newuser@EXAMPLE.ORG)
4. Create an appropriate keytab entry (adding to an existing file or creating a new one)

A new keytab file can be created with the following command:

~~~
kadmin ktadd -k /var/lib/irods/irods.keytab irodsserver/serverhost.example.org@EXAMPLE.ORG
~~~

#### iRODS Configuration

Configuring iRODS to communicate via Kerberos requires a few simple steps.

First, if Kerberos is being configured for a new user, it must be created:

~~~
iadmin mkuser newuser rodsuser
~~~

Then that user must be configured so its principal matches the KDC:

~~~
iadmin aua newuser newuser@EXAMPLE.ORG
~~~

The `/etc/irods/server_config.json` must be updated to include:

~~~
kerberos_service_principal=irodsserver/serverhost.example.org@EXAMPLE.ORG
kerberos_keytab=/var/lib/irods/irods.keytab
~~~

On the client side, the user's 'irods_auth_scheme' must be set to 'KRB'.  This can be done via environment variable:

~~~
irods@hostname:~/ $ irods_auth_scheme=KRB
irods@hostname:~/ $ export irods_auth_scheme
~~~

Or, preferably, in the user's `irods_environment.json` file:

~~~
"irods_auth_scheme": "KRB",
~~~

Then, to initialize the Kerberos session ticket and authenticate:

~~~
kinit
~~~

#### Limitations

The iRODS administrator will see two limitations when using Kerberos authentication:

1. The 'clientUserName' environment variable will fail (the admin cannot alias as another user)
2. The `iadmin moduser password` will fail (cannot update the user's password)

The workaround is to use iRODS native password authentication when using these.

`ipasswd` for rodsusers will also fail, but it is not an issue as it would be trying to update their (unused) iRODS native password.  They should not be updating their Kerberos passwords via iCommands.




### PAM

#### User Setup

PAM can be configured to to support various authentication systems; however the iRODS administrator still needs to add the users to the iRODS database:

~~~
irods@hostname:~/ $ iadmin mkuser newuser rodsuser
~~~

If the user's credentials will be exclusively authenticated with PAM, a password need not be assigned.

For PAM Authentication, the iRODS user selects the new iRODS PAM authentication choice (instead of password, or Kerberos) via their `irods_environment.json` file or by setting their environment variable:

~~~
irods@hostname:~/ $ irods_auth_scheme=PAM
irods@hostname:~/ $ export irods_auth_scheme
~~~

Then, the user runs 'iinit' and enters their system password.  To protect the system password, SSL (via OpenSSL) is used to encrypt the `iinit` session.




Since PAM requires the user's password in plaintext, iRODS relies on SSL encryption to protect these credentials.  PAM authentication makes use of SSL regardless of the iRODS Zone SSL configuration (meaning even if iRODS explicitly does *not* encrypt data traffic, PAM will use SSL during authentication).

In order to use the iRODS PAM support, you also need to have SSL working between the iRODS client and server. The SSL communication between client and iRODS server needs some basic setup in order to function properly. Much of the setup concerns getting a proper X.509 certificate setup on the server side, and setting up the trust for the server certificate on the client side. You can use either a self-signed certificate (best for testing) or a certificate from a trusted CA.



#### Server Configuration

The following keywords are used to set values for PAM server configuration.  These were previously defined as compile-time options.  They are now configurable via the `/etc/irods/server_config.json` configuration file.  The default values have been preserved.

- pam_password_length
- pam_no_extend
- pam_password_min_time
- pam_password_max_time

#### Server SSL Setup

Here are the basic steps to configure the server:

##### Generate a new RSA key

Make sure it does not have a passphrase (i.e. do not use the -des, -des3 or -idea options to genrsa):

~~~
irods@hostname:~/ $ openssl genrsa -out server.key
~~~

##### Acquire a certificate for the server

The certificate can be either from a trusted CA (internal or external), or can be self-signed (common for development and testing). To request a certificate from a CA, create your certificate signing request, and then follow the instructions given by the CA. When running the 'openssl req' command, some questions will be asked about what to put in the certificate. The locality fields do not really matter from the point of view of verification, but you probably want to try to be accurate. What is important, especially since this is a certificate for a server host, is to make sure to use the FQDN of the server as the "common name" for the certificate (should be the same name that clients use as their `irods_host`), and do not add an email address. If you are working with a CA, you can also put host aliases that users might use to access the host in the 'subjectAltName' X.509 extension field if the CA offers this capability.

To generate a Certificate Signing Request that can be sent to a CA, run the 'openssl req' command using the previously generated key:

~~~
irods@hostname:~/ $ openssl req -new -key server.key -out server.csr
~~~

To generate a self-signed certificate, also run 'openssl req', but with slightly different parameters. In the openssl command, you can put as many days as you wish:

~~~
irods@hostname:~/ $ openssl req -new -x509 -key server.key -out server.crt -days 365
~~~

##### Create the certificate chain file

If you are using a self-signed certificate, the chain file is just the same as the file with the certificate (server.crt).  If you have received a certificate from a CA, this file contains all the certificates that together can be used to verify the certificate, from the host certificate through the chain of intermediate CAs to the ultimate root CA.

An example best illustrates how to create this file. A certificate for a host 'irods.example.org' is requested from the proper domain registrar. Three files are received from the CA: irods.crt, PositiveSSLCA2.crt and AddTrustExternalCARoot.crt. The certificates have the following 'subjects' and 'issuers':

~~~
openssl x509 -noout -subject -issuer -in irods.crt
subject= /OU=Domain Control Validated/OU=PositiveSSL/CN=irods.example.org
issuer= /C=GB/ST=Greater Manchester/L=Salford/O=COMODO CA Limited/CN=PositiveSSL CA 2
openssl x509 -noout -subject -issuer -in PositiveSSLCA2.crt
subject= /C=GB/ST=Greater Manchester/L=Salford/O=COMODO CA Limited/CN=PositiveSSL CA 2
issuer= /C=SE/O=AddTrust AB/OU=AddTrust External TTP Network/CN=AddTrust External CA Root
openssl x509 -noout -subject -issuer -in AddTrustExternalCARoot.crt
subject= /C=SE/O=AddTrust AB/OU=AddTrust External TTP Network/CN=AddTrust External CA Root
issuer= /C=SE/O=AddTrust AB/OU=AddTrust External TTP Network/CN=AddTrust External CA Root
~~~

The irods.example.org cert was signed by the PositiveSSL CA 2, and that the PositiveSSL CA 2 cert was signed by the AddTrust External CA Root, and that the AddTrust External CA Root cert was self-signed, indicating that it is the root CA (and the end of the chain).

To create the chain file for irods.example.org:

~~~
irods@hostname:~/ $ cat irods.crt PositiveSSLCA2.crt AddTrustExternalCARoot.crt > chain.pem
~~~

##### Generate OpenSSL parameters

Generate some Diffie-Hellman parameters for OpenSSL:

~~~
irods@hostname:~/ $ openssl dhparam -2 -out dhparams.pem 2048
~~~

##### Place files within accessible area

Put the dhparams.pem, server.key and chain.pem files somewhere that the iRODS server can access them (e.g. in /etc/irods).  Make sure that the irods unix user can read the files (although you also want to make sure that the key file is only readable by the irods user).

##### Set SSL environment variables

The server needs to read these variables on startup:

~~~
irods@hostname:~/ $ irods_ssl_certificate_chain_file=/etc/irods/chain.pem
irods@hostname:~/ $ export irods_ssl_certificate_chain_file
irods@hostname:~/ $ irods_ssl_certificate_key_file=/etc/irods/server.key
irods@hostname:~/ $ export irods_ssl_certificate_key_file
irods@hostname:~/ $ irods_ssl_dh_params_file=/etc/irods/dhparams.pem
irods@hostname:~/ $ export irods_ssl_dh_params_file
~~~

##### Restart iRODS

Restart the server:

~~~
irods@hostname:~/ $ ./iRODS/irodsctl restart
~~~

#### Client SSL Setup

The client may or may not require configuration at the SSL level, but there are a few parameters that can be set via environment variables to customize the client SSL interaction if necessary. In many cases, if the server's certificate comes from a common CA, your system might already be configured to accept certificates from that CA, and you will not have to adjust the client configuration at all. For example, on an Ubuntu12 (Precise) system, the /etc/ssl/certs directory is used as a repository for system trusted certificates installed via an Ubuntu package. Many of the commercial certificate vendors such as VeriSign and AddTrust have their certificates already installed.

After setting up SSL on the server side, test SSL by using the PAM authentication (which requires an SSL connection) and running ``iinit`` with the log level set to LOG_NOTICE. If you see messages as follows, you need to set up trust for the server's certificate, or you need to turn off server verification.

Error from non-trusted self-signed certificate:

~~~
irods@hostname:~/ $ irods_log_level=LOG_NOTICE iinit
NOTICE: environment variable set, irods_log_level(input)=LOG_NOTICE, value=5
NOTICE: created irodsHome=/dn/home/irods
NOTICE: created irodsCwd=/dn/home/irods
Enter your current PAM (system) password:
NOTICE: sslVerifyCallback: problem with certificate at depth: 0
NOTICE: sslVerifyCallback:   issuer = /C=US/ST=North Carolina/L=Chapel Hill/O=RENCI/CN=irods.example.org
NOTICE: sslVerifyCallback:   subject = /C=US/ST=North Carolina/L=Chapel Hill/O=RENCI/CN=irods.example.org
NOTICE: sslVerifyCallback:   err 18:self signed certificate
ERROR: sslStart: error in SSL_connect. SSL error: error:14090086:SSL routines:SSL3_GET_SERVER_CERTIFICATE:certificate verify failed
sslStart failed with error -2103000 SSL_HANDSHAKE_ERROR
~~~

Error from untrusted CA that signed the server certificate:

~~~
irods@hostname:~/ $ irods_log_level=LOG_NOTICE iinit
NOTICE: environment variable set, irods_log_level(input)=LOG_NOTICE, value=5
NOTICE: created irodsHome=/dn/home/irods
NOTICE: created irodsCwd=/dn/home/irods
Enter your current PAM (system) password:
NOTICE: sslVerifyCallback: problem with certificate at depth: 1
NOTICE: sslVerifyCallback:   issuer = /C=US/ST=North Carolina/O=example.org/CN=irods.example.org Certificate Authority
NOTICE: sslVerifyCallback:   subject = /C=US/ST=North Carolina/O=example.org/CN=irods.example.org Certificate Authority
NOTICE: sslVerifyCallback:   err 19:self signed certificate in certificate chain
ERROR: sslStart: error in SSL_connect. SSL error: error:14090086:SSL routines:SSL3_GET_SERVER_CERTIFICATE:certificate verify failed
sslStart failed with error -2103000 SSL_HANDSHAKE_ERROR
~~~

Server verification can be turned off using the irodsSSLVerifyServer environment variable. If this variable is set to 'none', then any certificate (or none) is accepted by the client. This means that your connection will be encrypted, but you cannot be sure to what server (i.e. there is no server authentication). For that reason, this mode is discouraged.

It is much better to set up trust for the server's certificate, even if it is a self-signed certificate. The easiest way is to use the irods_ssl_ca_certificate_file environment variable to contain all the certificates of either hosts or CAs that you trust. If you configured the server as described above, you could just set the following in your environment:

~~~
irods@hostname:~/ $ irods_ssl_ca_certificate_file=/etc/irods/chain.pem
irods@hostname:~/ $ export irods_ssl_ca_certificate_file
~~~

Or this file could just contain the root CA certificate for a CA-signed server certificate.
Another potential issue is that the server certificate does not contain the proper FQDN (in either the Common Name field or the subjectAltName field) to match the client's 'irodsHost' variable. If this situation cannot be corrected on the server side, the client can set:

~~~
irods@hostname:~/ $ irods_ssl_verify_server=cert
irods@hostname:~/ $ export irods_ssl_verify_server
~~~

Then, the client library will only require certificate validation, but will not check that the hostname of the iRODS server matches the hostname(s) embedded within the certificate.

#### Environment Variables

All the environment variables used by the SSL support (both server and client side) are listed below:

irods_ssl_certificate_chain_file (server)
:       The file containing the server's certificate chain. The certificates must be in PEM format and must be sorted starting with the subject's certificate (actual client or server certificate), followed by intermediate CA certificates if applicable, and ending at the highest level (root) CA.

irods_ssl_certificate_key_file (server)
:       Private key corresponding to the server's certificate in the certificate chain file.

irods_ssl_dh_params_file (server)
:       The Diffie-Hellman parameter file location.

irods_ssl_verify_server (client)
:       What level of server certificate based authentication to perform. 'none' means not to perform any authentication at all. 'cert' means to verify the certificate validity (i.e. that it was signed by a trusted CA). 'hostname' means to validate the certificate and to verify that the irods_host's FQDN matches either the common name or one of the subjectAltNames of the certificate. 'hostname' is the default setting.

irods_ssl_ca_certificate_file (client)
:       Location of a file of trusted CA certificates in PEM format. Note that the certificates in this file are used in conjunction with the system default trusted certificates.

irods_ssl_ca_certificate_path (client)
:       Location of a directory containing CA certificates in PEM format. The files each contain one CA certificate. The files are looked up by the CA subject name hash value, which must be available. If more than one CA certificate with the same name hash value exist, the extension must be different (e.g. 9d66eef0.0, 9d66eef0.1, etc.).  The search is performed based on the ordering of the extension number, regardless of other properties of the certificates.  Use the 'c_rehash' utility to create the necessary links.



## Other Notes


The iRODS setting 'StrictACL' is configured on by default in iRODS 4.x.  This is different from iRODS 3.x and behaves more like standard Unix permissions.  This setting can be found in the `/etc/irods/core.re` file under acAclPolicy{}.



<!--
..
..
.. --------------
.. Best Practices
.. --------------
.. - tickets
.. - quota management
-->

## Configuration

### Configuration Files

There are a number of configuration files that control how an iRODS server behaves.  The following is a listing of the configuration files in an iRODS installation.

This document is intended to explain how the various configuration files are connected, what their parameters are, and when to use them.

~/.odbc.ini
:    This file, in the home directory of the unix service account (default 'irods'), defines the unixODBC connection details needed for the iCommands to communicate with the iCAT database.  This file was created by the installer package and probably should not be changed by the sysadmin unless they know what they are doing.

/etc/irods/database_config.json
:    This file defines the database settings for the iRODS installation.  It is created and populated by the installer package.

/etc/irods/server_config.json
:    This file defines the behavior of the server Agent that answers individual requests coming into iRODS.  It is created and populated by the installer package.

~/.irods/.irodsA
:    This is the scrambled password file that is saved after an ``iinit`` is run.  If this file does not exist, then each iCommand will prompt for a password before authenticating with the iRODS server.  If this file does exist, then each iCommand will read this file and use the contents as a cached password token and skip the password prompt.  This file can be deleted manually or can be removed by running ``iexit full``.

~/.irods/irods_environment.json
:    This is the main iRODS configuration file defining the iRODS environment.  Any changes are effective immediately since iCommands reload their environment on every execution.

### Checksum Configuration

Checksums in iRODS 4.0+ can be calculated using one of multiple hashing schemes.  Since the default hashing scheme for iRODS 4.0+ is SHA256, some existing earlier checksums may need to be recalculated and stored in the iCAT.

The following two settings, the default hash scheme and the default hash policy, need to be set on both the client and the server:

 | Client (irods_environment.json) | Server (server_config.json)    |
 | ------------------------------- | ------------------------------ |
 | irods_default_hash_scheme       | default_hash_scheme            |
 |  - SHA256 (default)             |  - SHA256 (default)            |
 |  - MD5                          |  - MD5                         |
 | ------------------------------- | ------------------------------ |
 | irods_match_hash_policy         | match_hash_policy              |
 |  - Compatible (default)         |  - Compatible (default)        |
 |  - Strict                       |  - Strict                      |

When a request is made, the sender and receiver's hash schemes and the receiver's policy are considered:

|  Sender      |   Receiver             |   Result                          |
| ------------ | ---------------------- | --------------------------------- |
|  MD5         |   MD5                  |   Success with MD5                |
|  SHA256      |   SHA256               |   Success with SHA256             |
|  MD5         |   SHA256, Compatible   |   Success with MD5                |
|  MD5         |   SHA256, Strict       |   Error, USER_HASH_TYPE_MISMATCH  |
|  SHA256      |   MD5, Compatible      |   Success with SHA256             |
|  SHA256      |   MD5, Strict          |   Error, USER_HASH_TYPE_MISMATCH  |

If the sender and receiver have consistent hash schemes defined, everything will match.

If the sender and receiver have inconsistent hash schemes defined, and the receiver's policy is set to 'compatible', the sender's hash scheme is used.

If the sender and receiver have inconsistent hash schemes defined, and the receiver's policy is set to 'strict', a USER_HASH_TYPE_MISMATCH error occurs.

### Special Characters

The default setting for 'standard_conforming_strings' in PostgreSQL 9.1+ was changed to 'on'.  Non-standard characters in iRODS Object names will require this setting to be changed to 'off'.  Without the correct setting, this may generate a USER_INPUT_PATH_ERROR error.

