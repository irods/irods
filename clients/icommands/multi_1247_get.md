# Multi-connection iget transport

For large files on iRODS 5.0 and newer, `iget` retains `DATA_OBJ_GET` for
resource and replica selection, checksum preparation, and completion. It
requests no portal, queries the selected descriptor, and downloads disjoint
ranges of the selected replica on independently authenticated connections to
the ordinary iRODS API port. The original checksum verification, local mode,
overwrite, progress and completion paths remain in effect. `-V` reports the
selected transport.

Keep the portal path for older or unrecognized server versions, small files,
`-N 0`, stdout, federation, public access, `--purgec`, special collections,
and large-file restart. If a stale or size-unknown replica is selected, use
the original sequential GET on the existing descriptor rather than assuming
the catalog size is an accurate segment boundary. Reads do not need a replica
token, but each worker must open the *selected* replica, not independently
select a different resource.

As with `iput`, a missing `-N` uses the client environment's default count
for the new transfer. Obtaining the server's `acSetNumThreads` decision
without starting a portal requires a future protocol extension. Keep
`getFileFromPortal` until all consumers and fallback paths have migrated;
remove the old portal protocol only as a separate, tested change.
