myTestRule {
#Input parameter is:
#  Path of file containing information
#Output parameter is:
#   Format of the file is
#   user-name|field|new-value
#   devtestuser|password|changemeplease
#  Status
  msiLoadUserModsFromDataObj(*Path,*Status);
  writeLine("stdout","Change password on a user account");
}
INPUT *Path="/tempZone/home/rods/test/load-usermods.txt"
OUTPUT ruleExecOut
