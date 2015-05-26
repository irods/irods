# Glossary

This glossary attempts to cover most of the terms you may encounter when first interacting with iRODS.


## Action

An external (logical) name given to an iRODS Rule(s) that defines a set of macro-level tasks.These tasks are performed by a chain of microservices in accordance with external input parameters. This is analogous to head atom in a Prolog rule or trigger-name in a relational database.</td>

## Agent

A type of iRODS server process.  Each time a client connects to a server, the server spawns an agent and a network connection is established between the new agent and the requesting client.

## API

An Application Programming Interface (API) is a piece of software's set of defined programmatic interfaces to enable other software to communicate with it.  iRODS defines a client API and expects that clients connect and communicate with iRODS servers in this controlled manner.  iRODS has an API written in C, and another written in Java (Jargon).  Other languages have wrappers around the C API (Python, PHP, etc.).

## Authentication Mechanisms

iRODS can employ various mechanisms to verify user identity and control access to Data Objects (iRODS files), Collections, etc.  These currently include the default iRODS secure password mechanism (challenge-response), Grid Security Infrastructure (GSI), Kerberos, and Operating System authentication (OSAuth).

## Audit Trail

List of all operations performed upon a Data Object, a Collection, a Resource, a User, or other iRODS entities.  When Auditing is enabled, significant events in the iRODS system (affecting the iCAT) are recorded.  Full activity reports can be compiled to verify important preservation and/or security policies have been enforced.

## Client

A Client in the iRODS client-server architecture gives users an interface to manipulate Data Objects and other iRODS entities that may be stored on remote iRODS servers. iRODS clients include: iCommands Unix-like command line utilities, iDrop (FTP-like client java application), iDropWeb (web interface), etc.

## Collection

All Data Objects stored in an iRODS system are stored in some Collection, which is a logical name for that set of Data Objects. A Collection can have sub-collections, and hence provides a hierarchical structure. An iRODS Collection is like a directory in a Unix file system (or Folder in Windows), but is not limited to a single device or partition. A Collection is logical so that the Data Objects can span separate and heterogeneous storage devices (i.e. is infrastructure and administrative domain independent). Each Data Object in a Collection must have a unique name in that Collection.

## Data Grid

A grid computing system (a set of distributed, cooperating computers) that deals with the controlled sharing and management of large amounts of distributed data.  An iRODS Grid consists of the physical computers, disks, network, and operating systems that run the iRODS software.  A Grid can report on its status, and member servers can be paused, resumed, or shutdown via the `irods-grid` command.

## Data Object

A Data Object is a single "stream-of-bytes" entity that can be uniquely identified and is stored in iRODS. It is given a Unique Internal Identifier in iRODS (allowing a global name space), and is associated with (situated in) a Collection.

## Resource Plugin

A piece of software that interfaces to a particular type of resource as part of the iRODS server/agent process. The plugin provides a common set of functions (open, read, write, close, etc.) which allow iRODS clients (iCommands and other programs using the client API) to access different devices via the common iRODS protocol.

## Federation

Zone Federation occurs when two or more independent iRODS Zones are registered with one another.  Users from one Zone can authenticate through their home iRODS server and have access rights on a remote Zone and its Data Objects, Collections, and Metadata.

## Jargon

The Java API for iRODS.  Read more at [https://github.com/DICE-UNC/jargon](https://github.com/DICE-UNC/jargon).

## iCAT

The iCAT, or iRODS Metadata Catalog, stores descriptive state metadata about the Data Objects in iRODS Collections in a DBMS database (e.g. PostgreSQL, MySQL, Oracle). The iCAT can keep track of both system-level metadata and user-defined metadata.  There is one iCAT database per iRODS Zone.

## iCAT Server (IES, or iCAT-Enabled Server)

The iRODS server in a Zone that holds the database connection to the (possibly remote) iCAT.

## iCommands

iCommands are Unix utilities that give users a command-line interface to operate on data in the iRODS system. There are commands related to the logical hierarchical filesystem, metadata, data object information, administration, rules, and the rule engine. iCommands provide the most comprehensive set of client-side standard iRODS manipulation functions.

## Inheritance

Collections in the iRODS logical name space have an attribute named Inheritance.  When Collections have this attribute set to Enabled, new Data Objects and Collections added to the Collection inherit the access permissions (ACLs) of the Collection. Data Objects created within Collections with Inheritance set to Disabled do not inherit the parent Collection's ACL settings.  `ichmod` can be used to manipulate this attribute on a per-Collection level.  `ils -A` displays ACLs and the inheritance status of the current working iRODS directory.

## Logical Name

The identifier used by iRODS to uniquely name a Data Object, Collection, Resource, or User. These identifiers enable global namespaces that are capable of spanning distributed storage and multiple administrative domains for shared Collections or a unified virtual Collection.

## Management Policies

The specification of the controls on procedures applied to Data Objects in a Collection. Management policies may define that certain Metadata be required to be stored or that a certain number of replicas be stored in particular physical locations.  Those policies could be implemented via a set of iRODS Rules that generate and verify the required Metadata.  Audit Trails could be used to generate reports that show that Management Policies have been followed.

## Metadata

Metadata is data about data.  In iRODS, metadata can include system or user-defined attributes associated with a Data-Object, Collection, Resource, etc., stored in the iCAT database.  The metadata stored in the iCAT database are in the form of AVUs (attribute-value-unit tuples).  Each of these three values is a string of characters in the database.  The flexibility of this system can be used to interface with many different metadata standards and templates across different information domains.

## Metadata Harvesting

The process of extraction of existing Metadata from a remote information resource and subsequent addition to the iRODS iCAT.  The harvested Metadata could be related to certain Data Objects, Collections, or any other iRODS entity.

## Microservice

Microservices are small, well-defined procedures/functions that perform a certain server-side task and are either compiled into the iRODS server code or packaged independently as shared objects ([Pluggable Microservices](manual#pluggable-microservices)). Rules invoke Microservices to implement Management Policies.  Microservices can be chained to implement larger macro-level functionality, called an Action. By having more than one chain of Microservices for an Action, a system can have multiple ways of performing the Action. At runtime, using priorities and validation conditions, the system chooses the "best" microservice chain to be executed.

## Migration

The process of moving digital Collections to new hardware and/or software as technology evolves.  Separately, Transformative Migration may be used to mean the process of manipulating a Data Object into a new format (e.g. gif to png) for preservation purposes.

## Physical Resource

A storage system onto which Data Objects may be deposited. iRODS supports a wide range of disk, tape, and remote storage resources.

## Resource

A resource, or storage resource, is a software/hardware system that stores digital data. iRODS clients can operate on local or remote data stored on different types of resources through a common interface.

## Resource Server

An iRODS server in a Zone that is *not* the iCAT Server.  There can be zero to many Resource Servers in a Zone.

## Rules

Rules are a major innovation in iRODS that let users automate data management tasks, essential as data collections scale to petabytes across hundreds of millions of files. Rules allow users to automate enforcement of complex Management Policies (workflows), controlling the server-side execution (via Microservices) of all data access and manipulation operations, with the capability of verifying these operations.

## Rule Engine

The Rule Engine interprets Rules following the iRODS rule syntax. The Rule Engine, which runs on all iRODS servers, is invoked by server-side procedure calls and selects, prioritizes, and applies Rules and their corresponding Microservices. The Rule Engine can apply recovery procedures if a Microservice or Action fails.

## Scalability

Scalability means that a computer system performs well, even when scaled up to very large sizes.  In iRODS, this refers to its ability to manage Collections ranging from the data on a single disk to petabytes (millions of gigabytes) of data in hundreds of millions of files distributed across multiple locations and administrative domains.

## Server

An iRODS server is software that interacts with the access protocol of a specific storage system.  It enables storing and sharing data distributed geographically and across administrative domains.

## Transformative Migration

The process of manipulating a Data Object from one encoding format to another.  Usually the target format will be newer and more compatible with other systems.  Sometimes this process is "lossy" and does not capture all of the information in the original format.

## Trust Virtualization

The management of Authentication and authorization independently of the data storage location(s).

## Unique Internal Identifier

See [Logical Name](#logical-name).

## User Name

Unique identifier for each person or entity using iRODS; sometimes combined with the name of the home iRODS Zone (as username#Zonename) to provide a globally unique name when using Zone Federation.

## Vault

An iRODS Vault is a data repository system that iRODS can maintain on any storage system which can be accessed by an iRODS server. For example, there can be an iRODS Vault on a Unix file system, an HPSS (High Performance Storage System), or a Ceph storage cluster. A Data Object in an iRODS Vault is stored as an iRODS-written object, with access controlled through the iCAT catalog. This is distinct from legacy data objects that can be accessed by iRODS but are still owned by previous owners of the data. For file systems such as Unix and HPSS, a separate directory is used.

## Zone

An iRODS Zone is an independent iRODS system consisting of an iCAT-Enabled Server (IES), optional additional distributed iRODS Resource Servers (which can reach hundreds, worldwide), and clients. Each Zone has a unique name. When two iRODS Zones are configured to interoperate with each other securely, it is called (Zone) Federation.  A Zone can report on its configuration and upgrade history via `izonereport`.
