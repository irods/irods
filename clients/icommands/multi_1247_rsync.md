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
`-N 0`, federation, ticket/public access, and other unsupported transfer
modes. For large iRODS-to-iRODS copies between different resource servers
in a local zone, the server-side copy/replication path can instead use
independent authenticated connections on the descriptor-owning server's API
port when both resource servers support iRODS 5.0+. It keeps the original
replica opens, policy, checksum handling and finalization; only the bytes
move without a portal.
This compatible path can relay bytes through the descriptor-owning server
rather than sending them directly from one resource server to the other.
The post-copy policy runs on that coordinating server (as for a single-thread
copy), rather than on the destination resource server as it does for some
parallel portal copies. Administrators with different rulebases on their
resource servers can use `-N 0` to retain the existing route. For operations
without `-N`, returning zero from the server's `acSetNumThreads` policy
selects the legacy route instead.
The legacy `rcDataObjRsync` behavior remains available to other callers.

The default number of workers without `-N` comes from the client environment.
Retrieving the server's `acSetNumThreads` policy result without creating a
portal requires a future protocol extension.
