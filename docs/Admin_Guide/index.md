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

