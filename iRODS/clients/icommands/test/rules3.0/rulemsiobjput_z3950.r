myTestRule {
#Input parameters are:
#  inMSOPath - the string sent to the remote URL
#  inCacheFilename - the full path of the cache file
#  inFileSize - the size of the cache file
#No output parameters
  msiSplitPath(*Path, *Coll, *File);
  msiExecStrCondQuery("SELECT DATA_SIZE where DATA_NAME = '*File' and COLL_NAME = '*Coll'",*GenQOut);
  foreach(*GenQOut) {
    msiGetValByKey(*GenQOut, "DATA_SIZE", *Size);
  }
  msiobjput_z3950(*Request, *Path, *Size);
}
INPUT *Request ="z3950:z3950.loc.gov:7090/Voyager?query=@attr 1=1003 Marx&recordsyntax=USMARC", *Path = "/tempZone/home/rods/sub1/rodsfile"
OUTPUT ruleExecOut
