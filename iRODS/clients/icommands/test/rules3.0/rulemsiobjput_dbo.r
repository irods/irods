myTestRule {
#Input parameters are:
#  inMSOPath - the string sent to the remote database object
#  inCacheFilename - the full path of the cache file
#  inFileSize - the size of the cache file
#No output parameters
  msiSplitPath(*Path, *Coll, *File);
  msiExecStrCondQuery("SELECT DATA_SIZE where DATA_NAME = '*File' and COLL_NAME = '*Coll'",*GenQOut);
  foreach(*GenQOut) {
    msiGetValByKey(*GenQOut, "DATA_SIZE", *Size);
  }
  msiobjput_dbo(*Request, *Path, *Size);
}
INPUT *Request ="dbo:dbr2:/tempzone/home/rods/dbotest/lt.pg", *Path = "/tempZone/home/rods/sub1/rodsfile"
OUTPUT ruleExecOut
