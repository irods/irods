# Best Practices

## Naming Conventions

When debugging a distributed, multi-layered system, it is very helpful to have 
things named well.  The iRODS convention has been to name things in the 
following format, `adjectiveNoun`.

Specific examples of this might include:

 - demoResc
 - replResc
 - rootResc
 - tempZone
 - renciZone

## Passthru resource as root node

Once a Zone has multiple users, changing certain Zone-wide settings becomes 
more difficult as every user must update their connection information before 
they can do any work with iRODS.  One of the ways to mitigate this difficulty 
is to use a passthru resource as the root node of the Zone's default resource.

By doing this, administrative changes to disks, server names, and resources can 
be handled out of view of the users and without the users needing to change any 
configuration in their client(s).

```
irods $ iadmin mkresc rootResc passthru
irods $ iadmin addchildtoresc rootResc unix1Resc
irods $ ilsresc
rootResc:passthru
└── unix1Resc
```

```
irods $ grep "^acSetRescSch" /etc/irods/core.re
acSetRescSchemeForCreate {msiSetDefaultResc("rootResc","null"); }
acSetRescSchemeForRepl {msiSetDefaultResc("rootResc","null"); }

```

## Do not use demoResc in production

iRODS is initially configured with the motivation of having a usable system.  To prepare a new installation for production, `demoResc` should be removed and replaced with a resource backed by a suitable production-quality storage system.  The Vault for `demoResc` is, by default, in the iRODS service account home directory (`/var/lib/irods/iRODS/Vault`) and not ready for production use.


<!--
..
.. - tickets
.. - quota management
-->

