myTestRule {
# Input parameters are:
#   inMSOPath - the string sent to the remote URL
#   inCacheFilename - the full path of the cache file
#   inFileSize - the size of the cache file, found from ls on the vault
# No output parameters
  msiobjput_http(*Request, *Path, *Size);
}
INPUT *Request ="//http://farm3.static.flickr.com/2254/5827459234_2fd1c55364_z.jpg", *Path = "/tempZone/home/rods/sub1/rodsfile", *Size = "15"
OUTPUT ruleExecOut
