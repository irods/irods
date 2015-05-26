# Backing Up

Backing up iRODS involves: The data, the iRODS system and configuration files, and the iCAT database itself.

Configuration and maintenance of this type of backup system is out of scope for this document, but is included here as an indication of best practice.

1. The data itself can be handled by the iRODS system through replication and should not require any specific backup efforts worth noting here.
2. The state of a zone (current policy and configuration settings) can be gathered by a 'rodsadmin' by running `izonereport`.  The resulting JSON file can be stored into iRODS or saved somewhere else.  When run on a regular schedule, the `izonereport` can gather all the necessary configuration information to help you reconstruct your iRODS setup during disaster recovery.
3. The iCAT database itself can be backed up in a variety of ways.  A PostgreSQL database is contained on the local filesystem as a data/ directory and can be copied like any other set of files.  This is the most basic means to have backup copies.  However, this will have stale information almost immediately.  To cut into this problem of staleness, PostgreSQL 8.4+ includes a feature called ["Record-based Log Shipping"](http://www.postgresql.org/docs/8.4/static/warm-standby.html#WARM-STANDBY-RECORD).  This consists of sending a full transaction log to another copy of PostgreSQL where it could be "re-played".  This would bring the copy up to date with the originating server.  Log shipping would generally be handled with a cronjob.  A faster, seamless version of log shipping called ["Streaming Replication"](http://www.postgresql.org/docs/9.0/static/warm-standby.html#STREAMING-REPLICATION) was included in PostgreSQL 9.0+ and can keep two PostgreSQL servers in sync with sub-second delay.


