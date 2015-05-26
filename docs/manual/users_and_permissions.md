# Users & Permissions

Users and permissions in iRODS are inspired by, but slightly different from, traditional Unix filesystem permissions.  Access to Data Objects and Collections can be modified using the `ichmod` iCommand.

## Groups

Additionally, permissions can be managed via user groups in iRODS.  A user can belong to more than one group at a time.  The owner of a Data Object has full control of the file and can grant and remove access to other users and groups.  The owner of a Data Object can also give ownership rights to other users, who in turn can grant or revoke access to users.

## Inheritance

Inheritance is a collection-specific setting that determines the permission settings for new Data Objects and sub-Collections.  Data Objects created within Collections with Inheritance set to Disabled do not inherit the parent Collection's permissions.  By default, iRODS has Inheritance set to Disabled.  More can be read from the help provided by `ichmod h`.

Inheritance is especially useful when working with shared projects such as a public Collection to which all users should have read access. With Inheritance set to Enabled, any sub-Collections created under the public Collection will inherit the properties of the public Collection.  Therefore, a user with read access to the public Collection will also have read access to all Data Objects and Collections created in the public Collection.

## StrictACL

The iRODS setting 'StrictACL' is configured on by default in iRODS 4.x.  This is different from iRODS 3.x and behaves more like standard Unix permissions.  This setting can be found in the `/etc/irods/core.re` file under acAclPolicy{}.

