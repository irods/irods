myTestRule {
# Input parameters are:
#   inMSOPath - the string sent to the remote database object
#   inCacheFilename - the full path of the cache file
#   inFileSize - the size of the cache file, found from ls on the vault
# No output parameters
  msiobjput_dbo(*Request, *Path, *Size);
}
INPUT *Request ="//dbo:dbr2:/tempzone/home/rods/dbotest/lt.pg", *Path = "/tempZone/home/rods/sub1/rodsfile", *Size = "15"
OUTPUT ruleExecOut
