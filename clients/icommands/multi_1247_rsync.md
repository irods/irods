# Multi-connection irsync transport

`irsync` retains its size/checksum comparison, age, dry-run, recursive, and
iRODS-to-iRODS logic. When a local-to-iRODS or iRODS-to-local transfer is
necessary and the server is at least 5.0.0, large files use the same
multi-connection API-port GET/PUT transports as `iget` and `iput`. This
includes transfers initiated by the `DATA_OBJ_RSYNC` server response; the
client still asks the server to decide whether a sync is necessary before
moving bytes. When that comparison is delegated to the server, `-l` obtains
the checksum decision without following a GET action, so listing cannot
download the file. `-V` reports the transport when a transfer occurs.

Portal transfers remain for older or unrecognized servers, small files,
`-N 0`, federation, ticket/public access, `irepl -a`, and other unsupported
transfer modes. For large iRODS-to-iRODS copies between different resource
servers in a local zone, the server-side copy/replication path can instead use
independent authenticated API-port connections when both resource servers
advertise `IRODS_LIBRARY_FEATURE_AUTHENTICATE_CLIENT`. If the destination
also advertises `IRODS_LIBRARY_FEATURE_SERVER_SIDE_MULTI_1247_COPY`, the
request is forwarded there *before* opening descriptors. The destination
owns the replica token,
and bytes travel directly from the source to the destination; its post-copy
policy runs on the destination as with the parallel portal path.

Destinations without the delegation feature can still use the portal-free
transport, but it relays bytes through the descriptor-owning server. The
post-copy policy runs on that coordinator (as for a single-thread copy),
rather than on the destination resource server as it does for some parallel
portal copies. Administrators with different rulebases on their resource
servers can use `-N 0` to retain the existing route. For operations without
`-N`, returning zero from `acSetNumThreads` selects the legacy route instead.
Both transports retain the original replica opens, checksum decisions and
finalization.
The legacy `rcDataObjRsync` behavior remains available to other callers.

The default number of workers without `-N` comes from the client environment.
Retrieving the server's `acSetNumThreads` policy result without creating a
portal requires a future protocol extension.
