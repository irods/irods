myTestRule {
# Input parameters are:
#   inRequestPath - the string sent to the remote iRODS data grid
#   inFileMode - the cache file creation mode
#   inFileFlags - the access modes for the cache file
#   inCacheFilename - the full path of the cache file on the local system
# No output parameters
# Output is the name of the file that was created
#   Wrote local file /tempZone/home/rods/sub1/rodsfileirods from remote //irods:iren.renci.org:1247:anonymous@renci/renci/home/rods/README.txt
  msiobjget_irods(*Request, *Mode, *Flags, *Path);
  writeLine("stdout","Wrote local file *Path from remote *Request");
}
INPUT *Request ="//irods:iren.renci.org:1247:anonymous@renci/renci/home/rods/README.txt", *Mode = "w", *Flags = "O_RDWR", *Path = "/tempZone/home/rods/sub1/rodsfileirods"
OUTPUT ruleExecOut
