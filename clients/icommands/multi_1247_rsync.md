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
`-N 0`, federation, and other unsupported transfer modes. iRODS-to-iRODS
copies remain on the server-side copy/replication path; eliminating their
server-to-server portals is separate work. The legacy `rcDataObjRsync`
behavior remains available to other callers.

The default number of workers without `-N` comes from the client environment.
Retrieving the server's `acSetNumThreads` policy result without creating a
portal requires a future protocol extension.
