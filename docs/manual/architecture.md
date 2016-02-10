# Architecture

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


## Dynamic Policy Enforcement Points

iRODS 4.0+ has introduced the capability for dynamic policy enforcement points (PEP).  For every operation that is called, two policy enforcement points are constructed (both a pre and post variety), and if it has been defined in core.re or any other loaded rulebase file they will be executed by the rule engine.

The PEP will be constructed of the form "pep_PLUGINOPERATION_pre" and "pep_PLUGINOPERATION_post".

For example, for "resource_create", the two PEPs that are dynamically evaluated are pep_resource_create_pre(\*OUT) and pep_resource_create_post(\*OUT).  If either or both have been defined in a loaded rulebase file (core.re), they will be executed as appropriate.

The flow of information from the pre PEP to the plugin operation to the post PEP works as follows:

- pep_PLUGINOPERATION_pre(\*OUT) - Should produce an \*OUT variable that will be passed to the calling plugin operation
- PLUGINOPERATION - Will receive any \*OUT defined by pep_PLUGINOPERATION_pre(\*OUT) above and will pass its own \*OUT variable to pep_PLUGINOPERATION_post()
- pep_PLUGINOPERATION_post() - Will receive any \*OUT from PLUGINOPERATION.  If the PLUGINOPERATION itself failed, the \*OUT variable will be populated with the string "OPERATION_FAILED".

Note that if pep_PLUGINOPERATION_pre() fails, the PLUGINOPERATION will not be called and the plugin operation call will fail with the resulting error code of the pep_PLUGINOPERATION_pre() rule call.  This ability to fail early allows for fine-grained control of which plugin operations may or may not be allowed as defined by the policy of the data grid administrator.

### Available Plugin Operations

The following operations are available for dynamic PEP evaluation.  At this time, only very few operations themselves consider the output (\*OUT) of its associated pre PEP. Also, not every plugin operation has an available network connection handle which is required for calling out to microservices, these are noted in the table.

<table border="1">
<tr><th>Plugin Type</th><th>Plugin Operation</th><th>Can Call Microservices</tr>
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
<td>
Yes<br />
Yes<br />
Yes<br />
Yes<br />
Yes<br />
Yes<br />
Yes<br />
Yes<br />
Yes<br />
Yes<br />
Yes
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
<td>
No<br />
No<br />
No<br />
No<br />
No<br />
No<br />
No<br />
No
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
<td>
No<br />
No<br />
No<br />
No<br />
No<br />
No<br />
No<br />
No
</td>
</tr>
<tr>
<td>Database</td>
<td>
database_start<br/>
database_stop<br/>
database_debug<br/>
database_open<br/>
database_close<br/>
database_get_local_zone<br/>
database_update_resc_obj_count<br/>
database_mod_data_obj_meta<br/>
database_reg_data_obj<br/>
database_reg_replica<br/>
database_unreg_replica<br/>
database_reg_rule_exec<br/>
database_mod_rule_exec<br/>
database_del_rule_exec<br/>
database_resc_obj_count<br/>
database_add_child_resc<br/>
database_reg_resc<br/>
database_del_child_resc<br/>
database_del_resc<br/>
database_rollback<br/>
database_commit<br/>
database_del_user_re<br/>
database_reg_coll_by_admin<br/>
database_reg_coll<br/>
database_mod_coll<br/>
database_simple_query<br/>
database_gen_query<br/>
database_gen_query_access_control_setup<br/>
database_gen_query_ticket_setup<br/>
database_specific_query<br/>
database_general_update<br/>
database_del_coll_by_admin<br/>
database_del_coll<br/>
database_check_auth<br/>
database_make_tmp_pw<br/>
database_make_limited_pw<br/>
database_mod_user<br/>
database_mod_group<br/>
database_mod_resc<br/>
database_mod_resc_data_paths<br/>
database_mod_resc_freespace<br/>
database_reg_user_re<br/>
database_add_avu_metadata<br/>
database_add_avu_metadata_wild<br/>
database_del_avu_metadata<br/>
database_set_avu_metadata<br/>
database_copy_avu_metadata<br/>
database_mod_avu_metadata<br/>
database_mod_access_control<br/>
database_mod_access_control_resc<br/>
database_rename_object<br/>
database_move_object<br/>
database_reg_token<br/>
database_del_token<br/>
database_reg_zone<br/>
database_mod_zone<br/>
database_mod_zone_coll_acl<br/>
database_del_zone<br/>
database_rename_local_zone<br/>
database_rename_coll<br/>
database_reg_server_load<br/>
database_reg_server_load_digest<br/>
database_purge_server_load<br/>
database_purge_server_load_digest<br/>
database_calc_usage_and_quota<br/>
database_set_quota<br/>
database_check_quota<br/>
database_del_unused_avus<br/>
database_add_specific_query<br/>
database_del_specific_query<br/>
database_version_rule_base<br/>
database_version_dvm_base<br/>
database_ins_rule_table<br/>
database_ins_dvm_table<br/>
database_ins_fnm_table<br/>
database_ins_msrvc_table<br/>
database_version_fnm_base<br/>
database_mod_ticket<br/>
database_update_pam_password<br/>
database_substitute_resource_hierarchies<br/>
database_get_distinct_data_objs_missing_from_child_given_parent<br/>
database_get_distinct_data_obj_count_on_resource<br/>
database_get_hierarchy_for_resc<br/>
database_check_and_get_obj_id<br/>
database_get_rcs
</td>
<td>
No<br/>
No<br/>
No<br/>
No<br/>
No<br/>
No<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
No<br/>
No<br/>
No<br/>
Yes<br/>
No<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
Yes<br/>
No<br/>
No<br/>
No<br/>
No
</td>
</tr>
</table>


Microservice plugins and API plugins are not wrapped with dynamic policy enforcement points.


### Available Values within Dynamic PEPs

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


<tr>
<td rowspan="3">Database</td>

<td>Postgres</td>
<td>No Values Available</td>
</tr>

<tr>
<td>MySQL</td>
<td>No Values Available</td>
</tr>

<tr>
<td>Oracle</td>
<td>No Values Available</td>
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

![ilsresc --tree](../treeview.png "alt title")

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

This compound resource auto-replication policy can be controlled with the context string associated with a compound resource.  The key "auto_repl" can have the value "on" (default), or "off".

For example, to turn off the automatic replication when creating a new compound resource (note the empty host/path parameter):

~~~
irods@hostname:~/ $ iadmin mkresc compResc compound '' auto_repl=off
~~~

When auto-replication is turned off, it may be necessary to replicate on demand.  For this scenario, there is a microservice named `msisync_to_archive()` which will sync (replicate) a data object from the child cache to the child archive of a compound resource.  This creates a new replica within iRODS of the synchronized data object.

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

The passthru resource was originally designed as a testing mechanism to exercise the new composable resource hierarchies.  They have proven to be more useful than that in a couple of interesting ways.

1. A passthru can be used as the root node of a resource hierarchy.  This will allow a Zone's users to have a stable default resource, even as an administrator changes out disks or other resource names in the Zone.

2. A passthru resource's contextString can be set to have an effect on its child's votes for both read and/or write.

To create a resource with priority read, use a 'read' weight greater than 1 (note the empty host:path parameter):

```
irods@hostname:~/ $ iadmin mkresc newResc passthru '' 'write=1.0;read=2.0'
Creating resource:
Name:           "newResc"
Type:           "passthru"
Host:           ""
Path:           ""
Context:        "write=1.0;read=2.0"
```

To modify an existing passthru resource to be written to only after other eligible resources, use a 'write' weight less than 1:

```
irods@hostname:~/ $ iadmin modresc newResc context 'write=0.4;read=1.0'
```

A passthru resource can have one and only one child.

#### Expected

A few other coordinating resource types have been brainstormed but are not functional at this time:

 - Storage Balanced (%-full) (expected)
 - Storage Balanced (bytes) (expected)
 - Tiered (expected)

### Storage Resources

Storage resources represent storage interfaces and include the file driver information to talk with different types of storage.

#### UnixFileSystem

The unixfilesystem storage resource is the default resource type that can communicate with a device through the standard POSIX interface.

A high water mark capability has been added to the unixfilesystem resource in 4.1.8.  The high water mark can be configured with the context string using the following syntax:

```
irods@hostname:~/ $ iadmin modresc unixResc context 'high_water_mark=1000'
```

The value is the total disk space used in bytes.  If a create operation would result in the total bytes on disk being larger than the high water mark, then the resource will return `USER_FILE_TOO_LARGE` and the create operation will not occur.  This feature allows administrators to protect their systems from absolute disk full events.  Writing to, or extending, existing file objects is still allowed.

#### Structured File Type (tar, zip, gzip, bzip)

The structured file type storage resource is used to interface with files that have a known format.  By default these are used "under the covers" and are not expected to be used directly by users (or administrators).

These are used mainly for mounted collections.

#### Amazon S3 (Archive)

The Amazon S3 archive storage resource is used to interface with an S3 bucket.  It is expected to be used as the archive child of a compound resource composition.  The credentials are stored in a file which is referenced by the context string.

Read more at: [https://github.com/irods/irods_resource_plugin_s3](https://github.com/irods/irods_resource_plugin_s3)

#### DDN WOS (Archive)

The DataDirect Networks (DDN) WOS archive storage resource is used to interface with a Web Object Scalar (WOS) Appliance.  It is expected to be used as the archive child of a compound resource composition.  It currently references a single WOS endpoint and WOS policy in the context string.

Read more at: [https://github.com/irods/irods_resource_plugin_wos](https://github.com/irods/irods_resource_plugin_wos)

#### HPSS

The HPSS storage resource is used to interface with an HPSS storage management system.  It can be used as the archive child of a compound resource composition or as a first class resource in iRODS.  The connection information is referenced in the context string.

Read more at: [https://github.com/irods/irods_resource_plugin_hpss](https://github.com/irods/irods_resource_plugin_hpss)

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
 - Pydap (expected)
 - TDS (expected)

### Managing Child Resources

There are two new `iadmin` subcommands introduced with this feature.

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

![example1 tree](../example1-tree.png)

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

The possible values for 'irods_client_server_policy' include:

- CS_NEG_REQUIRE: This side of the connection requires an SSL connection
- CS_NEG_DONT_CARE: This side of the connection will connect either with or without SSL
- CS_NEG_REFUSE: (default) This side of the connection refuses to connect via SSL

On the server side, the `core.re` has a default value of 'CS_NEG_DONT_CARE' in the acPreConnect() rule:

~~~
acPreConnect(*OUT) { *OUT="CS_NEG_DONT_CARE"; }
~~~

In order for a connection to be made, the client and server have to agree on the type of connection they will share.  When both sides choose `CS_NEG_DONT_CARE`, iRODS shows an affinity for security by connecting via SSL.  Additionally, it is important to note that all servers in an iRODS Zone are required to share the same SSL credentials (certificates, keys, etc.).  Maintaining per-route certificates is not supported at this time.

The remaining parameters are standard SSL parameters and made available through the EVP library included with OpenSSL.  You can read more about these remaining parameters at [https://www.openssl.org/docs/crypto/evp.html](https://www.openssl.org/docs/crypto/evp.html).

## Pluggable Database

The iRODS metadata catalog is now installed and managed by separate plugins.  The TEMPLATE_IRODSVERSION release has PostgreSQL, MySQL, and Oracle database plugins available and tested.

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
sudo apt-get install mysql-server mysql-client libpcre3-dev libmysqlclient-dev build-essential libtool autoconf git

# Build and Install
git clone https://github.com/mysqludf/lib_mysqludf_preg.git
cd lib_mysqludf_preg
git checkout lib_mysqludf_preg-1.1
autoreconf --force --install
./configure
make
sudo make install
sudo make MYSQL="mysql -p" installdb
sudo service mysql restart
~~~

The steps for installing `lib_mysqludf_preg` on CentOS 6 are:

~~~
# Get Dependencies
sudo yum install mysql mysql-server pcre-devel gcc make automake mysql-devel autoconf git

# Build and Install
git clone https://github.com/mysqludf/lib_mysqludf_preg.git
cd lib_mysqludf_preg
git checkout lib_mysqludf_preg-1.1
autoreconf --force --install
./configure
make
sudo make install
mysql --user=root --password="password" < installdb.sql
sudo service mysqld restart
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
