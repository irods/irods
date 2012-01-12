myTestRule {
#Input parameters are:
#  inMSOPath - the string that specifies a test of the msodrivers
#  inCacheFilename - the full path of the cache file
#  inFileSize - the size of the cache file
#No output parameters
  msiSplitPath(*Path, *Coll, *File);
  msiExecStrCondQuery("SELECT DATA_SIZE where DATA_NAME = '*File' and COLL_NAME = '*Coll'",*GenQOut);
  foreach(*GenQOut) {
    msiGetValByKey(*GenQOut, "DATA_SIZE", *Size);
  }
  msiobjput_test(*Request, *Path, *Size);
}
INPUT *Request ="test:Test string", *Path = "/tempZone/home/rods/sub1/rodsfile"
OUTPUT ruleExecOut
