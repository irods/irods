==================
iRODS-E 3.0 Manual
==================

.. contents:: Table of Contents
.. section-numbering::

----------------
ReStructuredText
----------------

Needs python modules::

 $ easy_install docutils==0.7.0
 $ easy_install roman
 $ easy_install rst2pdf

Some links for learning in place:

 http://docutils.sourceforge.net/docs/index.html

 http://docutils.sourceforge.net/docs/user/rst/cheatsheet.txt

 http://docutils.sourceforge.net/docs/user/rst/quickstart.txt

 http://docutils.sourceforge.net/docs/user/rst/quickstart.html

 http://docutils.sourceforge.net/docs/user/rst/demo.txt

 http://docutils.sourceforge.net/docs/user/rst/demo.html

 http://rst2pdf.googlecode.com/svn/trunk/doc/manual.txt

Generate HTML::

 $ rst2html.py -stg manual.rst > manual.html

Generate PDF::

 $ rst2pdf manual.rst -o manual.pdf

--------
Overview
--------

This manual attempts to provide standalone documentation for iRODS-E as packaged by the Renaissance Computing Institute (RENCI).

Additional documentation is available in the two books published by the iRODS team:

    (2010) iRODS Primer: integrated Rule-Oriented Data System (Synthesis Lectures on Information Concepts, Retrieval, and Services)
    http://www.amazon.com/dp/1608453332

    (2011) The integrated Rule-Oriented Data System (iRODS 3.0) Micro-service Workbook
    http://www.amazon.com/dp/1466469129


------------
Installation
------------

iRODS-E is released in binary form.  RPM and DEB formats are available for both iCAT-enabled servers and resource-only servers.  There are variations available for your combination of platform, operating system, and database type.

Installation of the x86-Ubuntu-Postgres RPM::

 $ rpm -i irodse-3.0-x86-ubuntu-postgres.rpm

Installation of the x86-CentOS-MySQL DEB::

 $ dpkg -i irodse-3.0-x86-centos-mysql.deb

----------
Quickstart
----------

Successful installation will complete and leave a running iRODS server.  If you installed an iCAT-enabled iRODS server, a database of your choice will also have been created and running.  The i-command ``ils`` will list your new iRODS administrator's empty home directory in the iRODS virtual filesystem::

 $ ils
 /tempZone/home/rods:
 $

-----------
Assumptions
-----------

iRODS-E enforces that the database in use (Postgres, MySQL, etc.) is configured for UTF-8 encoding.  For MySQL, this is enforced at the database level and the table level.  For Postgres, this is enforced at the database level and the tables inherit this setting.

-------------
Configuration
-------------



--------
Glossary
--------

This glossary attempts to cover most of the terms you may encounter when first interacting with iRODS.  More information can be found on the iRODS wiki at http://irods.org.

:Action:
    An Action is an external (logical) name given to an iRODS Rule(s) that defines a set of macro-level tasks.
    These tasks are performed by a chain of Micro-services in accordance with external input parameters (Attributes).
    Analogous to head atom in a Prolog rule or trigger-name in a relational database.

:Agent:
    An Agent is a type of iRODS server process.  Each time a client connects to a server, and agent is created and a network connection established between it and the client.

:Authentication Mechanisms:
    iRODS can employ various mechanisms to verify user identity and control access to data-Objects (iRODS files), Collections, etc.  These currently includes the default iRODS secure password mechanism (challenge-response), Grid Security Infrastructure (GSI), and Operating System authentication (OSAuth).

:Audit Trail:
    List of all operations performed upon a Data Object in a Collection or other iRODS entities; needed to establish authenticity.  When Auditing is enabled, significant events in the iRODS system (affecting the ICAT) are recorded. 

:Attribute:
    An entity that defines a property of an object, element, or file. Typically consists of a triplet: name, value, units. For example, a metadata attribute could be the running time of a digital video, with the attribute name in minutes. The triplet is included in the iCAT Metadata Catalog and associated with this particular video in the Collection. Also related to parameters or [[Attributes]] for [[Micro-services]].

:Bulk Operation:
    Manipulation of a group of Digital Objects, or a group of Users, or a group of storage systems.

:Client:
    A Client in the iRODS client-server architecture gives users an interface to manipulate datasets and other iRODS entities that may be stored on remote iRODS servers. iRODS clients include: i-commands unix-like command line interface, iRODS Web-based Browser, a graphical Web interface (A-grade browsers e.g. Internet Explorer 6+, Firefox 1.5+ and Safari 2.0+), iRODS Explorer for Windows, A simple Web client.

:Collection:
    All Data Objects stored in an iRODS/iCat system are stored in some Collection, which is a logical name for that set of data objects. A Collection can have sub-collections, and hence provides a hierarchical structure. An iRODS/iCAT Collection is like a directory in a Unix file system (or Folder in Windows), but is not limited to a single device or partition. A Collection is logical or  so that the data objects can span separate and heterogeneous storage devices (i.e. is infrastructure and administrative domain independent). Each Data Object in a Collection or sub-collection must have a unique name in that Collection. See [[logical name space]].

:Data Grid:
    A [http://en.wikipedia.org/wiki/Data_grid data grid] is a grid computing system (a set of distributed, cooperating computers) that deals with data — the controlled sharing and management of large amounts of distributed data.

:Data Object:
    A Data Object is a single a "stream-of-bytes" entity that can be uniquely identified, basically a file stored in iRODS. It is given a Unique Internal Identifier in iRODS (allowing a global name space), and is associated with a Collection (link). [what about compound objects?]

:Driver:
    A driver is software that interfaces to a particular type of resource as part of the iRODS Server/Agent process. The driver provides a common set of functions (open, read, write, close, etc.) which allow iRODS Clients (iCommands and other programs using the client API) to access different devices via the common iRODS protocol.

:Federation:
    An iRODS server is capable of communicating with other iRODS servers, in this way forming a Federation. Multiple iRODS federations can exist, with some aware of others. A user with appropriate permissions can access data objects stored under any iRODS/iCAT system in the Federation. As with the SRB, iRODS developers plan to release an iCAT federation capability, in which one iCAT-enabled iRODS system (Zone) can be integrated with others (Zone Federation). Each iCAT member of such a Zone Federation can share some or all of the iCAT metadata in a wide range of architectures.

:iCAT:
    The iRODS Metadata Catalog, which stores descriptive state metadata about the data objects in iRODS Collections in a DBMS database (e.g. PostgreSQL, Oracle). Metadata can be both system level and user defined. iCAT also refers to the library routines that interface to the iCAT database. The iCAT API is those routine names that begin with 'chl' for Catalog High Level interface. There are also mid-level ('cml') and low level ('cll') routines called by the High Level. See the [[icatAttributes]] page for more on the ICAT database.

:IES, iCAT-Enabled Server:
    A full iRODS installation, including an iRODS Server and associated iCAT Metadata Catalog, along with clients.

:iCommands:
    iCommands are Unix and Windows utilities that give users a command-line interface to operate on data in the iRODS system. There are Unix-like commands; Metadata-related commands, Informational, Administrative, and Rules and Delayed Rule Execution commands. iCommands provide the most comprehensive set of standard manipulation functions. There are also Web-based, GUI, and a growing number of other interfaces to iRODS.   

:Logical Name:
    The identifier used by iRODS to uniquely name a file, resource, or user. These identifiers enable global Name Spaces that are capable of spanning distributed storage and multiple administrative domains for Shared Collections or a unified Virtual Collection.

:Management Policies:
    The specification of the controls on procedures applied to records in a collection. Management policies lead to required Metadata, leading to iRODS Rules to generate and verify this Metadata.

:Metadata:
    Metadata is data about data.  In iRODS metadata includes system and user-defined attributes associated with a Data-Object, Collection, Resource, etc, stored in the iCAT database. To implement a persistent archives, preservation policies require approximately 200 metadata attributes.  

:Metadata Harvesting:
    Extraction of Metadata from a remote information resource and addition to metadata catalog, e.g. iCAT.

:Micro-service:
    Set of operations performed on a Collection at a remote storage location. Rules invoke Micro-services to implement Management Policies. 
Small, well-defined procedures/functions that perform a certain server-side task. iRODS Micro-services are compiled into the iRODS server code. They can be chained to implement a larger macro-level functionality. macro-level functionality. These macro-level functionalities are called actions. By having more than one chain of micro-services for an action, a system can have multiple ways of performing the action. Using priorities and validation conditions, at run-time, the system chooses the 'best' micro-service chain to be executed. There are other caveats to this execution paradigm which are discussed in the section on actions.
The task that is performed by a micro-service can be quite small or very involved. We leave it to the micro-service developer to choose the proper level of granularity for their task differentiation. A good rule of thumb would be to divide a large task into sub-tasks with well-defined interfaces and make each into a micro-service.

:Migration:
    Process of moving digital Collections to new hardware and software as technology evolves; see also Transformative Migration. 

:Name Space:
    Logical identifiers applied to Digital Objects in a Data Grid. The identifiers can be logically organized into a collection hierarchy. iRODS uses a unique name for each digital entity, allowing a global Namespace and unified virtual collections that can span multiple storage systems and organizations.  See [[logical name space]].

:Namespace:
    Reserved names used to specify Attributes for a specific information Schema.

:Persistent State:
    The information generated when a micro-service is executed, managed persistently across all operations. Implies persistent Name Spaces. 

:Physical Resource:
    Storage system into which Digital Data may be deposited. iRODS supports a wide range of tape and disk storage. See Driver. 

:Resource:
    A resource, or storage resource, is a software/hardware system that stores digital data. Currently iRODS can use a Unix file system as a resource. As other iRODS drivers are written, additional types of resources will be included. iRODS clients can operate on local or remote data stored on different types of resources, through a common interface. 

:Rules:
    Rules are a major innovation in iRODS that let users automate data management tasks, essential as data collections scale to petabytes in hundreds of millions of files. Rules allow users to automate enforcement of complex Management Policies (workflows), controlling the server-side execution (as micro-services) of all data access and manipulation operations, with the capability of verifying these operations.

:Rule Engine:
    The Rule Engine interprets Rules following the iRODS rule syntax (see Rules). The Rule Engine, which runs on all iRODS Servers, is invoked by server-side procedure calls and selects, prioritizes, and applies Rules and their corresponding Micro-services (link). The Rule Engine can apply recovery procedures if a Micro-service or Action fails.

:Scalability:
    Scalability means that a computer system performs well, even when scaled up to very large sizes.  In iRODS this refers to its ability to manage Collections ranging from a single PC to petabytes (millions of gigabytes) of data in hundreds of millions of files distributed across multiple locations and administrative domains. Makes use of the scalability of the underlying database system (through the ICAT) and requires Automation capabilities as in iRODS Rules.

:Server:
    An iRODS Server is software that interacts with the access protocol of a specific storage system; enables storing and sharing data distributed geographically and across administrative domains.

:Transformative Migration:
    Migration of an encoding format from one data Format to a different, usually newer format.

:Trust Virtualization:
    The management of Authentication and authorization independently of the storage location.

:Unique Internal Identifier:
    See Logical Name. 

:User Name:
    Unique identifier for each person using iRODS; sometimes combined with #Zonename to provide a globally unique name.  

:Vault:
    An iRODS vault is a data repository system that iRODS can maintain on any storage system which can be accessed by an iRODS server. For example, there can be an iRODS vault on a Unix file system, an HPSS (High Performance Storage System), or an IBM DB2 database. A data object in an iRODS vault is stored as an iRODS-written object, with access controlled through the iCAT catalog. This is distinct from legacy data objects that can be accessed by iRODS but are still owned by previous owners of the data. For file systems such as Unix and HPSS, a separate directory is used; for databases such as Oracle or DB2 a system-defined table with LOB-space is used. 

:Virtualization:
    Manage the properties of a system by managing the global Namespaces (see Namespace, Trust Virtualization)

:Zone:
    An iRODS Zone is an independent iRODS system consisting of an iCAT-enabled server, optional additional distributed iRODS Servers (which can reach hundreds, worldwide) and clients. Each Zone has a unique name. When two iRODS Zones are configured to interoperate with each other securely, it is called Federation.


-------------------
History of Releases
-------------------

==========   =======    =====================================================
Date         Version    Description
==========   =======    =====================================================
2012-03-01   3.0        Initial Release.
                         This is the first release from RENCI, based on the
                         iRODS 3.0 community codebase.
==========   =======    =====================================================

