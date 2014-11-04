# Overview

## Key Data Management Tasks and iRODS

iRODS, the Integrated Rule-Oriented Data System, is a state-of-the-art open 
source software system for addressing the key data management tasks that face 
users as the size and complexity of digital data collections continue to grow 
rapidly. Because the principal data management tasks are highly interrelated, 
rather than taking a piecemeal approach or addressing just a single task, the 
iRODS system takes a comprehensive approach to full data life cycle management.

At the same time, the system design is highly user-driven and avoids the 
pitfalls of a "one size fits all" design by building on a comprehensive generic 
platform with a highly configurable architecture. In addition, iRODS offers 
multiple paths to interoperation with outside systems such as repositories, 
interfaces, and applications. This lets users adapt iRODS to the details of 
their own environment in a wide range of production applications that can 
emphasize different aspects of data management in diverse domains.

### Small scale use

iRODS can be downloaded and fully installed in a simple default configuration 
in a few minutes. For example, one user is applying iRODS in a stand-alone 
installation running on a single laptop computer, using external hard drives 
for data storage, in oceanographic research that simultaneously harvests 
multiple types of data from instruments on a research vessel in the Antarctic, 
far from shore-based support. Another institution is using iRODS to preserve 
periodic Web crawls to maintain a long-term historical view of their Web-based 
information.

### Large scale use

#### Sharing and Preserving Data for Collaboration in a Data Grid

Because iRODS grew out of a supercomputing environment, it scales to very large use. Big 
scientific research projects such as the Ocean Observatories Initiative (OOI); 
the National Climatic Data Center (NCDC); the National Optical Astronomy 
Observatory (NOAO); the Australian Research Collaboration Service (ARCS); and 
the Centre de Calcul de l'Institut National de Physique Nucléaire et de 
Physique des Particules (CC-IN2P3) in France are using iRODS for managing 
petybytes of data in hundreds of millions of files on distributed storage 
resources spread across the country and around the globe.

#### Preservation Environments for Long-term Archiving

The [National Archives and Records Administration (NARA) Transcontinental Persistent Archives Prototype 
(TPAP)](http://www.archives.gov/applied-research/tpap.html), and the National Library of France are examples of large-scale archival 
applications of iRODS.

## Why Was iRODS Developed?

Recent decades have seen a rapid rise in collaborative activities in scientific 
research, and more broadly across many sectors of society. Driven by new 
information technologies such as the Web as well as the increasing complexity 
and interdisciplinary nature of today's research problems – from climate 
change to biomedicine, large-scale astronomical projects, and the world's 
increasingly integrated economies – are driving the need for technologies, 
sometimes called "cyberinfrastructure," that enable researchers to collaborate 
effectively continues to grow rapidly. The Integrated Rule-Oriented Data System 
(iRODS) is state-of-the-art software that supports collaborative research, and 
more broadly management, sharing, publication, and long-term preservation of 
data that are distributed.

A tool for collaboration, iRODS is itself the product of a fruitful 
collaboration spanning more than two decades among high performance computing, 
preservation, and library communities, whose real-world needs have driven and 
shaped iRODS development. The computational science and high performance 
computing (HPC) communities are inherently interdisciplinary, generating and 
using very large data collections distributed across multiple sites and groups. 
The massive size of these data collections has encouraged development of unique 
capabilities in iRODS that allow scaling to collections containing petabytes of 
data and hundreds of millions of files.

The preservation community brings the need for long-term preservation of 
digital information, a challenging problem that is still an active research 
area to which iRODS research activities have made significant contributions. 
Interestingly, there turn out to be significant commonalities in the 
requirements for preserving digital data in time and collaborative sharing of 
distributed data collections across space, whether geographic, institutional, 
disciplinary, etc. The third community that has contributed to iRODS 
development is the library community, with expertise in descriptive metadata 
that is essential for management, discovery, repurposing, as well as controlled 
sharing and long-term preservation of digital collections.

In collaborating with these communities, iRODS research and development has 
been characterized by close attention to the practical requirements of a wide 
range of users, resulting in pioneering architecture and solutions to numerous 
distributed data challenges that now form the iRODS Data System for managing, 
sharing, publishing, and preserving today's rapidly growing and increasingly 
complex digital collections.

The integrated Rule Oriented Data System (iRODS) is software middleware, or 
"cyberinfrastructure," that organizes distributed data into a sharable 
collection. The iRODS software is used to implement a data grid that assembles 
data into a logical collection. Properties such as integrity (uncorrupted 
record), authenticity (linking of provenance information to each record), chain 
of custody (tracking of location and management controls within the 
preservation environment), and trustworthiness (sustainability of the records) 
can be imposed on the logical collection. When data sets are distributed across 
multiple types of storage systems, across multiple administrative domains, 
across multiple institutions, and across multiple countries, data grid 
technology is used to enforce uniform management policies on the assembled 
collection.

The specific challenges addressed by iRODS include:

- Management of interactions with storage resources that use different access 
protocols. The data grid provides mechanisms to map from the actions requested 
by a client to the protocol required by a specific vendor supplied disk, tape, 
archive, or object-relational database.

- Support for authentication and authorization across systems that use 
different identity management systems. The data grid authenticates all access, 
and authorizes and logs all operations upon the files registered into the 
shared collection.

- Support for uniform management policies across institutions that may have 
differing access requirements such as different Institutional Research Board 
approval processes. The policies controlling use, distribution, replication, 
retention, disposition, authenticity, integrity, and trustworthiness are 
enforced by the data grid.

- Support for wide-area-network access. To maintain interactive response, 
network transport is optimized for moving massive files (through parallel I/O 
streams), for moving small files (through encapsulation of the file in the 
initial data transfer request), for moving large numbers of small files 
(aggregation into containers), and for minimizing the amount of data sent over 
the network (execution of remote procedures such as data subsetting on each 
storage resource).

In response to these challenges, iRODS is an ongoing research and software 
development effort to provide software infrastructure solutions that enable 
collaborative research. The software systems are implemented as middleware that 
interacts with remote storage systems on behalf of the users. The goal of the 
iRODS team is to develop generic software that can be used to implement all 
distributed data management applications, through changing the management 
policies and procedures. This has been realized by creating a highly extensible 
software infrastructure that can be modified without requiring the modification 
of the core software or development of new software code.

