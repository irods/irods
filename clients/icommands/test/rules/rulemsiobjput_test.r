myTestRule {
# Input parameters are:
#   inMSOPath - the string that specifies a test of the microservice object framework
#   inCacheFilename - the full path of the cache file in the local storage vault
#   inFileSize - the size of the cache file, found from ls on the vault
# No output parameters
  msiobjput_test(*Request, *Path, *Size);
}
INPUT *Request ="//test:Test string", *Path = "/tempZone/home/rods/sub1/rodsfile", *Size = "15"
OUTPUT ruleExecOut
