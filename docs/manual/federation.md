# Zone Federation

iRODS Zones are independent administrative units.  When federated, users of one Zone may grant access to authenticated users from the other Zone on some of their dataObjects, Collections, and Metadata.  Each Zone will authenticate its own users before a Federated Zone will allow access.  User passwords are never exchanged between Zones.

Primary reasons for using Zone Federation include:

1. Local control. Some iRODS sites want to share resources and collections, yet maintain more local control over those resources, data objects, and collections. Rather than a single iRODS Zone managed by one administrator, they may need two (or more) cooperating iRODS systems managed locally, primarily for security and/or authorization reasons.
2. iCAT WAN performance. In world-wide networks, the network latency may cause significant iRODS performance degradation. For example, in the United States, the latency between the east coast and the west coast is often 1-2 seconds for a simple query. Many iRODS operations require multiple interations with the iCAT database, which compounds any delays.

## Setup

To federate Zone A and Zone B, administrators in each zone must:

1. Coordinate and share their `icat_host`, `zone_name`, `zone_key`, and `negotiation_key` information
2. Define the remote zone in their respective iCAT, and
3. Define any remote users in their respective iCAT before any access permissions can be granted.


In Zone A, add Zone B and define a remote user:

~~~
ZoneA $ iadmin mkzone ZoneB remote zoneB-iCAT.hostname.example.org:ZoneBPort
ZoneA $ iadmin mkuser bobby#ZoneB rodsuser
~~~

In Zone B, add Zone A, but skip adding any remote users at this time:

~~~
ZoneB $ iadmin mkzone ZoneA remote zoneA-iCAT.hostname.example.org:ZoneAPort
~~~

Then, any user of Zone A will be able to grant permissions to `bobby#ZoneB`:

~~~
ZoneA $ ichmod read bobby#ZoneB myFile
~~~

Once permission is granted, it will appear like any other ACL:

~~~
ZoneA $ ils -A myFile
  /ZoneA/home/rods/myFile
        ACL - bobby#ZoneB:read object   rods#ZoneA:own
~~~

If all the [Server Authentication](#server-authentication) and the networking is set up correctly, Bobby can now `iget` the shared Zone A file from Zone B:

~~~
ZoneB $ iget /ZoneA/home/rods/myFile
~~~

## Server Authentication

### Within A Zone

When a client connects to a resource server and then authenticates, the resource server connects to the iCAT server to perform the authentication. To make this more secure, you must configure the `zone_key` to cause the iRODS system to authenticate the servers themselves. This `zone_key` should be a unique and arbitrary string (maximum alphanumeric length of 49), one for your whole zone:

~~~
"zone_key": "SomeChosenKeyString",
~~~

This allows the resource servers to verify the identity of the iCAT server beyond just relying on DNS.

Mutual authentication between servers is always on.  Note that this applies to iRODS passwords and PAM, and some other interactions, but not GSI or Kerberos.

For GSI, users can set the `irodsServerDn` variable to do mutual authentication.

### Between Two Zones

When a user from a remote zone connects to the local zone, the iRODS server will check with the iCAT in the user's home zone to authenticate the user (confirm their password).  Passwords are never sent across federated zones, they always remain in their home zone.

To make this more secure, the iRODS system uses both the `zone_key` and the `negotiation_key` to authenticate the servers in `server_config.json`, via a similar method as iRODS passwords. The `zone_key` should be a unique and arbitrary string, one for each zone.  The `negotiation_key` should be a shared key only for this pairing of two zones.

To configure this, add items to the `/etc/irods/server_config.json` file.

`zone_key` is for the servers [Within A Zone](#within-a-zone), for example:

~~~
"zone_key": "asdf1234",
~~~

Add an object to the "federation" array for each remote zone, for example:

~~~
"federation": [
    {
        "icat_host": "otherzone.example.org",
        "zone_name": "anotherZone",
        "zone_key": "ghjk6789",
        "negotiation_key": "abcdefghijklmnopqrstuvwxyzabcdef"
    }
]
~~~

When anotherZone users connect, the system will then confirm that anotherZone's `zone_key` is 'ghjk6789'.

Mutual authentication between servers is always on across Federations.

## Federation with iRODS 3.x

iRODS 4.0+ has made some additions to the database tables for the resources (`r_resc_main`) and Data Objects (`r_data_main`) for the purposes of tracking resource hierarchy, children, parents, and other relationships.  These changes would have caused a cross-zone query to fail when the target zone is iRODS 3.x.

In order to support commands such as `ils` and `ilsresc` across a 3.x to 4.0+ federation, iRODS 4.0+ will detect the cross zone query and subsequently strip out any requests for columns which do not exist in the iRODS 3.x table structure in order to allow the query to succeed.

There are currently no known issues with Federation, but this has not yet been comprehensively tested.

### irods_environment.json for Service Account

`irods_client_server_negotiation` needs to be changed (set it to "none") as 3.x does not support this feature.

The effect of turning this negotiation off is a lack of SSL encryption when talking with a 3.x Zone.  All clients that connect to this 4.0+ Zone will also need to disable the Advanced Negotiation in their own 'irods_environment.json' files.

