myTestRule {
#Input parameter is:
#  Path of file containing accounts to delete
#    Format of file is
#     user-name|
#      guest|
#     assumes the zone is the same as the zone of the client
#Output parameter is:
#  Status
  msiDeleteUsersFromDataObj(*Path,*Status);
  writeLine("stdout","User accounts deleted as specified in file *Path");
}
INPUT *Path="/tempZone/home/rods/testcoll/rodsaccountdelete"
OUTPUT ruleExecOut
