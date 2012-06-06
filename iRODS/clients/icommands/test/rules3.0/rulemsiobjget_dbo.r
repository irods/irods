myTestRule {
# Input parameters are:
#   inRequestPath - the string sent to the remote database object
#   inFileMode - the cache file creation mode
#   inFileFlags - the access modes for the cache file
#   inCacheFilename - the full path of the cache file on the local system
# No output parameters
# Output is the name of the file that was created
#   Wrote local file /tempZone/home/rods/sub1/rodsfiledbo from remote //dbo:dbr2:/tempZone/home/rods/dbotest/lt.pg
  msiobjget_dbo(*Request, *Mode, *Flags, *Path);
  writeLine("stdout","Wrote local file *Path from remote *Request");
}
INPUT *Request ="//dbo:dbr2:/tempZone/home/rods/dbotest/lt.pg", *Mode = "w", *Flags = "O_RDWR", *Path = "/tempZone/home/rods/sub1/rodsfiledbo"
OUTPUT ruleExecOut
