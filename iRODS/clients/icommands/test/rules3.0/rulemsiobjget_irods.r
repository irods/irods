myTestRule {
#Input parameters are:
#  inRequestPath - the string sent to the remote iRODS data grid
#  inFileMode - the cache file creation mode
#  inFileFlags - the access modes for the cache file
#  inCacheFilename - the full path of the cache file
#No output parameters
#Output is the name of the file that was created
  msiSplitPath(*Path, *Coll, *File);
  msiobjget_irods(*Request, *Mode, *Flags, *Path);
  msiExecStrCondQuery("SELECT DATA_NAME where DATA_NAME = '*File' and COLL_NAME = '*Coll'",*GenQOut);
  foreach(*GenQOut) {
    msiGetValByKey(*GenQOut, "DATA_NAME", *Filestore);
    writeLine("stdout","Created file *Filestore");
  }
}
INPUT *Request ="irods:iren.renci.org:1247:anonymous@renci/renci/home/rods/README.txt", *Mode = "w", *Flags = "O_RDWR", *Path = "/tempZone/home/rods/sub1/rodsfile"
OUTPUT ruleExecOut
