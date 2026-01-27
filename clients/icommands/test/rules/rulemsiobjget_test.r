myTestRule {
# Input parameters are:
#   inRequestPath - the string that specifies a test of the microservice object framework
#   inFileMode - the cache file creation mode
#   inFileFlags - the access modes for the cache file
#   inCacheFilename - the full path of the cache file on the local system
# No output parameters
# Output is the name of the file that was created
#   Wrote local file /tempZone/home/rods/sub1/rodsfiletest from remote //test:Test string
  msiobjget_test(*Request, *Mode, *Flags, *Path);
  writeLine("stdout","Wrote local file *Path from remote *Request");
}
INPUT *Request ="//test:Test string", *Mode = "r+", *Flags = "O_RDWR", *Path = "/tempZone/home/rods/sub1/rodsfiletest"
OUTPUT ruleExecOut
