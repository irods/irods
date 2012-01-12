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
  msiobjput_http(*Request, *Path, *Size);
}
INPUT *Request ="http://farm3.static.flickr.com/2254/5827459234_2fd1c55364_z.jpg", *Path = "/tempZone/home/rods/sub1/rodsfile"
OUTPUT ruleExecOut
