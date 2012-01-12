The SQL script in this directory can be run manually (via psql) after
irods installation or as part of the installation process (by
responding 'yes' to the appropriate question in 'irodssetup').

When running manually, you will need to change the 'rodsbuild' to the
user name of the owner of the ICAT database (irodssetup does this for
you).

This SQL system supports advanced logging.  Essentially, it creates a
look-up table that translates numeric iRODS action ids into human
readable action names (ext_audit_actions).  NCCS then built a separate
Java web application that consults the ICAT for all logged action ids
and displays the associated action names. This gives the administrator
an extended view (vw_ext_audit) of the activities associated with each
file.

To also keep a history of deleted artifacts, NCCS also built shadow
tables to store the related history (ext_<artifact>_historical).
Recorded artifacts include deleted collections, zones, resources, data
(e.g., files).  Also created were views (vw_<artifact>_audit) that
collect and publish the history for each of these tables. Finally,
NCCS created a trigger for each table which causes the history to be
recorded when one of the artifacts is deleted.

Currently, only Postgres is supported.

This was donated by the
NASA Center for Climate Simulation (NCCS)
NASA / Goddard Space Flight Center
Greenbelt, MD 20771

For additional information, contact Glenn Tamkin (Computer Science
Corp) at <glenn.s.tamkin@nasa.gov>
