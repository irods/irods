# Multi-connection iput transport

The `iput` put lifecycle remains the existing `DATA_OBJ_PUT` API: resource
selection, permissions, overwrite checks, policy, checksum verification,
metadata/ACL application, and `rcOprComplete` all stay on the primary
connection. For sufficiently large files on servers at least 5.0.0, the
client suppresses the portal and transfers only the bytes via independently
authenticated connections to the configured iRODS port. The primary replica
token authorizes secondary opens; each worker writes a disjoint range and
closes without updating the catalog. The original put completes after all
workers close. `-V` identifies the selected byte transport.

Older servers, unrecognized versions, `-N 0`, small files, federation, and
large-file restart retain the original put implementation. These are not
changed by the new transport. Test both paths when adjusting put behavior.
The new route uses the client environment's default transfer thread count
when `-N` is omitted. Existing 5.0 servers cannot report their
`acSetNumThreads` result without also opening a portal. A future protocol
extension could return this policy decision separately.

Future removal of portals should be staged separately: cover federation,
special collections and restartable transfers, migrate the other portal
clients (`iget`, `irsync`, etc.), and only then delete `putFileToPortal`, the
server portal handlers, and their protocol structures. Do not remove the
legacy transfer merely because `iput` can use the new path locally.
