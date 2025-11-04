myTestRule {
# Input parameters are:
#   inRequestPath - the string representing the symbolic link to dereference
#   inFileMode - the cache file creation mode
#   inFileFlags - the access modes for the cache file
#   inCacheFilename - the full path of the cache file on the local system
# No output parameters
# Output is the name of the file that was created
#   Wrote local file /tempZone/home/rods/sub1/rodsfileslink from remote //slink:/renci/home/rods/README.txt
  msiobjget_slink(*Request, *Mode, *Flags, *Path);
  writeLine("stdout","Wrote local file *Path from remote *Request");
}
INPUT *Request ="//slink:/renci/home/rods/README.txt", *Mode = "w", *Flags = "O_RDWR", *Path = "/tempZone/home/rods/sub1/rodsfileslink"
OUTPUT ruleExecOut
