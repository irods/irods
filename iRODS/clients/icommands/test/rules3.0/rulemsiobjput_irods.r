myTestRule {
#Input parameters are:
#  inMSOPath - the string sent to the remote iRODS data grid
#  inCacheFilename - the full path of the cache file
#  inFileSize - the size of the cache file
#No output parameters
  msiSplitPath(*Path, *Coll, *File);
  msiExecStrCondQuery("SELECT DATA_SIZE where DATA_NAME = '*File' and COLL_NAME = '*Coll'",*GenQOut);
  foreach(*GenQOut) {
    msiGetValByKey(*GenQOut, "DATA_SIZE", *Size);
  }
  msiobjput_irods(*Request, *Path, *Size);
}
INPUT *Request ="irods:iren.renci.org:1247:anonymous@renci/renci/home/rods/README.txt", *Path = "/tempZone/home/rods/sub1/rodsfile"
OUTPUT ruleExecOut
