#!/bin/sh
#

# Convenience script to check that the local copy of scripts and input
# files are current (or how they differ).  I decided it was probably
# better to do this by hand once in a while instead of automatically
# trying to use some of these from within the tar file (can't get them
# all that way due to the bootstraping-like problem)).

set -x 

diff irods1.sh IRODS_BUILD/iRODS/server/test/tbox
diff irods2.sh IRODS_BUILD/iRODS/server/test/tbox
diff input1.txt.no.pg IRODS_BUILD/iRODS/server/test/tbox
diff input1.txt.reuse.pg IRODS_BUILD/iRODS/server/test/tbox
diff start_tinderbox IRODS_BUILD/iRODS/server/test/tbox
