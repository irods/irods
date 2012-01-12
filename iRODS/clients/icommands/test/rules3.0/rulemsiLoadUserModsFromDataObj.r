myTestRule {
#Input parameter is:
#  Path of file containing information
#Output parameter is:
#   Format of the file is
#   user-name|field|new-value
#   guest|password|guest1
#  Status
  msiLoadUserModsFromDataObj(*Path,*Status);
  writeLine("stdout","Change password on a user account");
}
INPUT *Path="/tempZone/home/rods/testcoll/rodsaccountmod"
OUTPUT ruleExecOut
