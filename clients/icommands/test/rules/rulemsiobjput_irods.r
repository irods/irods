myTestRule {
# Input parameters are:
#   inMSOPath - the string sent to the remote iRODS data grid
#   inCacheFilename - the full path of the cache file
#   inFileSize - the size of the cache file, found from ls on the vault
# No output parameters
  msiobjput_irods(*Request, *Path, *Size);
}
INPUT *Request ="//irods:iren.renci.org:1247:anonymous@renci/renci/home/rods/README.txt", *Path = "/tempZone/home/rods/sub1/rodsfile", *Size = "15"
OUTPUT ruleExecOut
