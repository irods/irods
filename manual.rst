=================================
Enterprise iRODS (E-iRODS) Manual
=================================

:Author: Renaissance Computing Institute (RENCI)
:Version: 3.0beta2

.. contents:: Table of Contents
.. section-numbering::

.. 
..   ----------------
..   ReStructuredText
..   ----------------
.. 
..   Needs python modules::
.. 
..    $ easy_install docutils==0.7.0
..    $ easy_install roman
..    $ easy_install rst2pdf
.. 
..   Some links for learning in place:
.. 
..    http://docutils.sourceforge.net/docs/index.html
.. 
..    http://docutils.sourceforge.net/docs/user/rst/cheatsheet.txt
.. 
..    http://docutils.sourceforge.net/docs/user/rst/quickstart.txt
.. 
..    http://docutils.sourceforge.net/docs/user/rst/quickstart.html
.. 
..    http://docutils.sourceforge.net/docs/user/rst/demo.txt
.. 
..    http://docutils.sourceforge.net/docs/user/rst/demo.html
.. 
..    http://rst2pdf.googlecode.com/svn/trunk/doc/manual.txt
.. 
..   Generate HTML::
.. 
..    $ rst2html.py -stg manual.rst > manual.html
.. 
..   Generate PDF::
.. 
..    $ rst2pdf manual.rst -o manual.pdf

--------
Overview
--------

This manual attempts to provide standalone documentation for E-iRODS as packaged by the Renaissance Computing Institute (RENCI).

    http://e-irods.com

    file:///var/lib/e-rods/iRODS/doc/html/index.html

Additional documentation is available on the iRODS wiki and in the two books published by the iRODS team:

    http://irods.org

    http://irods.org/doxygen

    (2010) iRODS Primer: integrated Rule-Oriented Data System (Synthesis Lectures on Information Concepts, Retrieval, and Services)
    http://www.amazon.com/dp/1608453332

    (2011) The integrated Rule-Oriented Data System (iRODS 3.0) Micro-service Workbook
    http://www.amazon.com/dp/1466469129


--------
Download
--------

E-iRODS is released in binary form.  RPM and DEB formats are available for both iCAT-enabled servers and resource-only servers.  There are variations available for combinations of platform, operating system, and database type.

More combinations will be made available as our testing matrix increases.

The latest files can be downloaded from http://e-irods.org/download.


------------
Installation
------------

Installation of the Postgres iCAT DEB::

 $ (sudo) dpkg -i e-irods-3.0b2-64bit-icat-postgres.deb

Installation of the Resource RPM::

 $ (sudo) rpm -i e-irods-3.0b2-64bit-resource.rpm

These packages install the dependencies necessary to run E-iRODS, a service account and group named 'eirods', the E-iRODS binaries, microservice documentation, and this manual.

For the iCAT-enabled server packages, the E-iRODS server and EICAT database are started automatically with default values::

 $ eirods@hostname:~> ienv
 NOTICE: Release Version = rods3.0, API Version = d
 NOTICE: irodsHost=hostname
 NOTICE: irodsPort=1247
 NOTICE: irodsDefResource=demoResc
 NOTICE: irodsHome=/tempZone/home/rods
 NOTICE: irodsCwd=/tempZone/home/rods
 NOTICE: irodsUserName=rods
 NOTICE: irodsZone=tempZone 

For the resource-only packages, the server is not started automatically.  The administrator will need to run the packaging/setup_resource.sh script and provide the following three pieces of information before E-iRODS can start and connect to its configured iCAT Zone:

1) Hostname or IP
2) iCAT Port
3) iCAT Zone 


----------
Quickstart
----------

Successful installation will complete and leave a running iRODS server.  If you installed an iCAT-enabled iRODS server, a database of your choice will also have been created and running.  The iCommand ``ils`` will list your new iRODS administrator's empty home directory in the iRODS virtual filesystem::

 $ eirods@hostname:~> ils
 /tempZone/home/rods:

When moving into production, you will probably want to cover the next few basic steps::

Changing the administrator account password
-------------------------------------------

The default installation of E-iRODS comes with a single account 'rods' that has rodsadmin privileges and password 'rods'.  You should change the password before letting anyone else onto the system::

 $ eirods@hostname:~> iadmin moduser rods password <newpassword>

To make sure everything succeeded, you'll need to reauthenticate and check the new connection::
 
 $ eirods@hostname:~> iinit
 Enter your current iRODS password:
 $ eirods@hostname:~> ils
 /tempZone/home/rods:

Changing the Zone name
----------------------

The default installation of E-iRODS comes with a Zone named 'tempZone'.  You probably want to change the Zone name to something more domain-specific::

 $ eirods@hostname:~> iadmin modzone tempZone name <newzonename>
 If you modify the local zone name, you and other users will need to
 change your .irodsEnv files to use it, you may need to update
 irods.config and, if rules use the zone name, you'll need to update
 core.re.  This command will update various tables with the new name
 and rename the top-level collection.
 Do you really want to modify the local zone name? (enter y or yes to do so):y
 OK, performing the local zone rename
 
The Zone has been renamed, but now you will need to update your .irodsEnv file to match (note the three places where the updated zone name is located)::

 $ eirods@hostname:~> cat .irods/.irodsEnv
 # iRODS server host name:
 irodsHost 'ubuntu2'
 # iRODS server port number:
 irodsPort 1247
 # Default storage resource name:
 irodsDefResource 'demoResc'
 # Home directory in iRODS:
 irodsHome '/<newzonename>/home/rods'
 # Current directory in iRODS:
 irodsCwd '/<newzonename>/home/rods'
 # Account name:
 irodsUserName 'rods'
 # Zone:
 irodsZone '<newzonename>'

Now, the connection should be reset and you should be able to list your empty iRODS collection again::

 $ eirods@hostname:~> iinit
 Enter your current iRODS password:
 $ eirods@hostname:~> ils
 /<newzonename>/home/rods:

Add additional resource(s)
--------------------------

The default installation of E-iRODS comes with a single resource named 'demoResc' and which stores its files in the /var/lib/e-irods/iRODS/Vault directory.  You will want to create additional resources at disk locations of your choosing.  The following command will create a basic 'cache' resource at a designated host at the designated fullpath::

 $ eirods@hostname:~> iadmin mkresc <newrescname> 'unix file system' cache <fully.qualified.domain.name> </full/path/to/new/vault>
 
Additional information about creating resources can be found with::

 $ eirods@hostname:~> iadmin help mkresc
  mkresc Name Type Class Host [Path] (make Resource)
 Create (register) a new storage or database resource.

 Name is the name of the new resource.
 Type is the resource type (see 'lt resc_type' for a list).
 Class is the usage class of the resource (see 'lt resc_class').
 Host is the DNS host name.
 And Path is the defaultPath for the vault (not needed for resources of
 type 'database' (DBRs)).

 Tip: Also see the lt command for Type and Class token information.

 $ eirods@hostname:~> iadmin lt resc_type
 unix file system 
 hpss file system 
 windows file system 
 s3 
 MSS universal driver 
 database 
 mso 

 $ eirods@hostname:~> iadmin lt resc_class
 cache 
 archive 
 compound 
 bundle 
 postgresql 
 mysql 
 oracle 
  
Creating new resources does not make them default for any existing or new users.  You will need to make sure that default resources are properly set for newly ingested files.

Add additional user(s)
----------------------

The default installation of E-iRODS comes with a single user 'rods' which is a designated 'rodsadmin' type user account.  You will want to create additional 'rodsuser' type user accounts and set their passwords before allowing connections to your new grid::

 $ eirods@hostname:~> iadmin mkuser <newusername> rodsuser 

 $ eirods@hostname:~> iadmin lu
 rodsBoot#tempZone
 rods#tempZone
 <newusername>#tempZone
 
 $ eirods@hostname:~> iadmin help mkuser
  mkuser Name[#Zone] Type (make user)
 Create a new iRODS user in the ICAT database

 Name is the user name to create
 Type is the user type (see 'lt user_type' for a list)
 Zone is the user's zone (for remote-zone users)

 Tip: Use moduser to set a password or other attributes,
 use 'aua' to add a user auth name (GSI DN or Kerberos Principal name)

Best practice suggests changing your Zone name before adding new users as any existing users would need to be informed of the new connection information and changes that would need to be made to their local .irodsEnv files.

---------
Upgrading
---------

The beta release of E-iRODS does not yet support upgrading.  Every install will be a clean install.

This section will be updated when support is included.

----------
Backing Up
----------

Backing up E-iRODS consists of three major parts:  The data, the iRODS system and configuration files, and the iCAT database itself.

1) The data itself can be handled by the iRODS system through replication and should not require any specific backup efforts worth noting here.

2) The iRODS system and configuration files can be copied into iRODS as a set of Data Objects by using the `msiServerBackup`_ microservice.  When run on a regular schedule, the `msiServerBackup` microservice will contain all the necessary configuration information to reconstruct your iRODS setup during disaster recovery.

.. _msiServerBackup: file:///var/lib/e-irods/iRODS/doc/html/sys_backup_m_s_8c_abab044dfcae659a200741d4f69999c29.html

3) The iCAT database itself can be backed up in a variety of ways.  A Postgres database is contained on the local filesystem as a data/ directory and can be copied like any other set of files.  This is the most basic means to have backup copies.  However, this will have stale information almost immediately.  To cut into this problem of staleness, Postgres 8.4 includes a feature called `"Record-based Log Shipping"`__.  This consists of sending a full transaction log to another copy of Postgres where it could be "re-played" and bring the copy up to date with the originating server.  Log shipping would generally be handled with a cronjob.  A faster, seamless version of log shipping called `"Streaming Replication"`__ was included in Postgres 9.0+ and can keep two Postgres servers in sync with sub-second delay.

.. __: http://www.postgresql.org/docs/8.4/static/warm-standby.html#WARM-STANDBY-RECORD
.. __: http://www.postgresql.org/docs/9.0/static/warm-standby.html#STREAMING-REPLICATION

Configuration and maintenance of this type of backup system is out of scope for this document, but is included here as an indication of best practice.

-----------
Assumptions
-----------

E-iRODS enforces that the database in use (Postgres, MySQL, etc.) is configured for UTF-8 encoding.  For MySQL, this is enforced at the database level and the table level.  For Postgres, this is enforced at the database level and then the tables inherit this setting.  MySQL is not yet supported with a binary release.

The iRODS setting 'StrictACL' is configured on by default in E-iRODS.  This is different from the community version of iRODS and behaves more like standard Unix permissions.  This setting can be found in the `server/config/reConfigs/core.re` file under acAclPolicy{}.


.. 
.. ------
.. How To
.. ------
.. 
.. Troubleshooting
.. ---------------
.. 
.. These will be cross-referenced with each feature.
.. 
.. - where to check
.. - what to expect
.. - error codes - with numeric to string translation
.. 
.. Common Errors
.. -------------
.. iRODS Server is down
.. credentials
.. file not found
.. port/firewall
.. wrong server/port
.. client version mismatch
.. rule engine syntax
.. iRODS permissions
.. 
.. Steps
.. -----
.. - ienv
.. - networking
..  - reachable?
..  - port open?
..  - server up?
.. - check logs
..  - on server
..  - on client
.. 
.. Manage Resources
.. ----------------
.. - cache
..   - cache cleanup (itrim via cronjob)
..   - monitoring
.. - compound
.. - database
.. - WOS
.. - HPSS
.. - S3
.. - Group Population
..    - Random
..    - Round Robin
..    - Least Populated
.. 
.. Manage Users
.. ------------
.. - groups
.. - ACLs (always surprising)
..    - multiple people / groups
..    - inheritance
..    - StrictACL
.. 
.. Examples
.. --------
.. - Least Recently Used (LRU)
.. - First In First Out (FIFO)
.. - Failover checking
.. 
.. ----------
.. Monitoring
.. ----------
.. - nagios plugins (Jean-Yves)
.. - other
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
.. 
.. --------------
.. Authentication
.. --------------
.. - iRODS
.. - OSAuth
.. - GSI
.. 
.. --------------
.. Best Practices
.. --------------
.. - microservice objects (MSO)
.. - tickets
.. - realizable objects
.. - quota management

-------------
Configuration
-------------

There are a number of configuration files that control how an iRODS server behaves.  The following is a listing of the configuration files in a binary-only E-iRODS installation.

This document is intended to explain how the various configuration files are connected, what their parameters are, and when to use them.

~/.odbc.ini

iRODS/config/irods.config

iRODS/server/config/server.config

~/.irods/.irodsA
    This is the scrambled password file that is saved after an ``iinit`` is run.  If this file does not exist, then each iCommand will prompt for a password before authenticating with the iRODS server.  If this file does exist, then each iCommand will read this file and use the contents as a cached password token and skip the password prompt.  This file can be deleted manually or can be removed by running ``iexit full``.

~/.irods/.irodsEnv
    This is the main iRODS configuration file defining the iRODS environment.  Any changes are effective immediately since iCommands reload their environment on every execution.


--------
Glossary
--------

This glossary attempts to cover most of the terms you may encounter when first interacting with iRODS.  More information can be found on the iRODS wiki at http://irods.org.

Action
    An external (logical) name given to an iRODS Rule(s) that defines a set of macro-level tasks.
    These tasks are performed by a chain of Micro-services in accordance with external input parameters.
    Analogous to head atom in a Prolog rule or trigger-name in a relational database.

Agent
    A type of iRODS server process.  Each time a client connects to a server, and agent is created and a network connection established between it and the client.

API
    An Application Programming Interface (API) is a piece of software's set of defined programmatic interfaces to enable other software to communicate with it.  iRODS defines a client API and expects that clients connect and communicate with iRODS servers in this controlled manner.  iRODS has an API written in C, and another written in Java (Jargon). 

Authentication Mechanisms
    iRODS can employ various mechanisms to verify user identity and control access to Data Objects (iRODS files), Collections, etc.  These currently includes the default iRODS secure password mechanism (challenge-response), Grid Security Infrastructure (GSI), and Operating System authentication (OSAuth).

Audit Trail
    List of all operations performed upon a Data Object, a Collection, a Resource, a User, or other iRODS entities.  When Auditing is enabled, significant events in the iRODS system (affecting the iCAT) are recorded.  Full activity reports can be compiled to verify important preservation and/or security policies have been enforced.

Client
    A Client in the iRODS client-server architecture gives users an interface to manipulate Data Objects and other iRODS entities that may be stored on remote iRODS servers. iRODS clients include: iCommands unix-like command line interface, iDrop (ftp-like client java application), iDropWeb (web interface), etc.

Collection
    All Data Objects stored in an iRODS system are stored in some Collection, which is a logical name for that set of Data Objects. A Collection can have sub-collections, and hence provides a hierarchical structure. An iRODS Collection is like a directory in a Unix file system (or Folder in Windows), but is not limited to a single device or partition. A Collection is logical so that the Data Objects can span separate and heterogeneous storage devices (i.e. is infrastructure and administrative domain independent). Each Data Object in a Collection must have a unique name in that Collection.

Data Grid
    A grid computing system (a set of distributed, cooperating computers) that deals with the controlled sharing and management of large amounts of distributed data.

Data Object
    A Data Object is a single "stream-of-bytes" entity that can be uniquely identified; a file stored in iRODS. It is given a Unique Internal Identifier in iRODS (allowing a global name space), and is associated with (situated in) a Collection.

Driver
    A piece of software that interfaces to a particular type of resource as part of the iRODS server/agent process. The driver provides a common set of functions (open, read, write, close, etc.) which allow iRODS clients (iCommands and other programs using the client API) to access different devices via the common iRODS protocol.

Federation
    Zone Federation occurs when two or more independent iRODS Zones are registered with one another.  Users from one Zone can authenticate through their home iRODS server and have access rights on a remote Zone and its Data Objects, Collections, and Metadata.

Jargon
    The Java API for iRODS.  Read more at https://www.irods.org/index.php/Jargon.
iCAT
    The iCAT, or iRODS Metadata Catalog, stores descriptive state metadata about the Data Objects in iRODS Collections in a DBMS database (e.g. PostgreSQL, MySQL, Oracle). The iCAT can keep track of both system-level metadata and user-defined metadata.  There is one iCAT database per iRODS Zone.

IES (iCAT-Enabled Server)
    A machine that runs both an iRODS server and the iCAT database for a particular Zone.

iCommands
    iCommands are Unix utilities that give users a command-line interface to operate on data in the iRODS system. There are commands related to the logical hierarchical filesystem, metadata, data object information, administration, rules, and the rule engine. iCommands provide the most comprehensive set of client-side standard iRODS manipulation functions.

Inheritance
    Collections in the iRODS logical name space have an attribute named Inheritance.  When Collections have this attribute set to Enabled, new Data Objects and Collections added to the Collection inherit the access permissions (ACLs) of the Collection. Data Objects created within Collections with Inheritance set to Disabled do not inherit the parent Collection's ACL settings.  ``ichmod`` can be used to manipulate this attribute on a per-Collection level.  ``ils -A`` displays ACLs and the inheritance status of the current working iRODS directory.

Logical Name
    The identifier used by iRODS to uniquely name a Data Object, Collection, Resource, or User. These identifiers enable global namespaces that are capable of spanning distributed storage and multiple administrative domains for shared Collections or a unified virtual Collection.

Management Policies
    The specification of the controls on procedures applied to Data Objects in a Collection. Management policies may define that certain Metadata be required to be stored.  Those policies could be implemented via a set of iRODS Rules that generate and verify the required Metadata.  Audit Trails could be used to generate reports that show that Management Policies have been followed.

Metadata
    Metadata is data about data.  In iRODS, metadata can include system or user-defined attributes associated with a Data-Object, Collection, Resource, etc., stored in the iCAT database.  The metadata stored in the iCAT database are in the form of AVUs (attribute-value-unit tuples).

Metadata Harvesting
    The process of extraction of existing Metadata from a remote information resource and subsequent addition to the iRODS iCAT.  The harvested Metadata could be related to certain Data Objects, Collections, or any other iRODS entity.

Micro-service
    A set of operations performed on a Collection at a remote storage location. 

    Micro-services are small, well-defined procedures/functions that perform a certain server-side task and are compiled into the iRODS server code. Rules invoke Micro-services to implement Management Policies.  Micro-services can be chained to implement larger macro-level functionality, called an Action. By having more than one chain of Micro-services for an Action, a system can have multiple ways of performing the Action. At runtime, using priorities and validation conditions, the system chooses the "best" micro-service chain to be executed. 

Migration
    The process of moving digital Collections to new hardware and/or software as technology evolves.  Separately, Transformative Migration may be used to mean the process of manipulating a Data Object into a new format (e.g. gif to png) for preservation purposes.

Physical Resource
    A storage system onto which Data Objects may be deposited. iRODS supports a wide range of disk, tape, and remote storage resources.

Resource
    A resource, or storage resource, is a software/hardware system that stores digital data. Resources can be classified as cache, archive, or compound (a virtual type consisting of a cache resource affiliated with an archive resource).  iRODS clients can operate on local or remote data stored on different types of resources through a common interface.

Rules
    Rules are a major innovation in iRODS that let users automate data management tasks, essential as data collections scale to petabytes across hundreds of millions of files. Rules allow users to automate enforcement of complex Management Policies (workflows), controlling the server-side execution (via Micro-services) of all data access and manipulation operations, with the capability of verifying these operations.

Rule Engine
    The Rule Engine interprets Rules following the iRODS rule syntax. The Rule Engine, which runs on all iRODS servers, is invoked by server-side procedure calls and selects, prioritizes, and applies Rules and their corresponding Micro-services. The Rule Engine can apply recovery procedures if a Micro-service or Action fails.

Scalability
    Scalability means that a computer system performs well, even when scaled up to very large sizes.  In iRODS, this refers to its ability to manage Collections ranging from the data on a single disk to petabytes (millions of gigabytes) of data in hundreds of millions of files distributed across multiple locations and administrative domains.

Server
    An iRODS server is software that interacts with the access protocol of a specific storage system.  It enables storing and sharing data distributed geographically and across administrative domains.

Transformative Migration
    The process of manipulating a Data Object from one encoding format to another.  Usually the target format will be newer and more compatible with other systems.  Sometimes this process is "lossy" and does not capture all of the information in the original format.

Trust Virtualization
    The management of Authentication and authorization independently of the storage location.

Unique Internal Identifier
    See Logical Name. 

User Name
    Unique identifier for each person or entity using iRODS; sometimes combined with the name of the home iRODS Zone (as username#Zonename) to provide a globally unique name when using Zone Federation.

Vault
    An iRODS Vault is a data repository system that iRODS can maintain on any storage system which can be accessed by an iRODS server. For example, there can be an iRODS Vault on a Unix file system, an HPSS (High Performance Storage System), or an IBM DB2 database. A Data Object in an iRODS Vault is stored as an iRODS-written object, with access controlled through the iCAT catalog. This is distinct from legacy data objects that can be accessed by iRODS but are still owned by previous owners of the data. For file systems such as Unix and HPSS, a separate directory is used; for databases such as Oracle or DB2 a system-defined table with LOB-space (Large Object space) is used. 

Zone
    An iRODS Zone is an independent iRODS system consisting of an iCAT-Enabled Server (IES), optional additional distributed iRODS Servers (which can reach hundreds, worldwide) and clients. Each Zone has a unique name. When two iRODS Zones are configured to interoperate with each other securely, it is called (Zone) Federation.


-------------------
History of Releases
-------------------

==========   =======    =====================================================
Date         Version    Description
==========   =======    =====================================================
2012-06-25   3.0b2      Second Beta Release.
                          This is the second release from RENCI.  It includes
                          packages for iCAT, Resource, iCommands, and
                          development, in both DEB and RPM formats.
                          Also includes more documentation.
2012-03-01   3.0b1      Initial Beta Release.
                          This is the first release from RENCI, based on the
                          iRODS 3.0 community codebase.
==========   =======    =====================================================

