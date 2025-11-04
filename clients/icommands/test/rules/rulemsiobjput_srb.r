myTestRule {
# Input parameters are:
#   inMSOPath - the string sent to the remote Storage Resource Broker data grid
#   inCacheFilename - the full path of the cache file
#   inFileSize - the size of the cache file, found from ls on the vault
# No output parameters
  msiobjput_srb(*Request, *Path, *Size);
}
INPUT *Request ="//srb:srbbrick11.sdsc.edu:7676:testuser@sdsc:TESTUSER/UCHRI/home/srbAdmin.uchri/testdir/testFile", *Path = "/tempZone/home/rods/sub1/rodsfile", *Size = "15"
OUTPUT ruleExecOut
