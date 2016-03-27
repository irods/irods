myTestRule {
#Input parameters are:
#  Path
#   File format for user accounts is
#     User-name|User-ID|User-type|Zone|
#     guest|001|rodsuser|tempZone   
#Output parameter is:
#  Status
  msiCreateUserAccountsFromDataObj(*Path,*Status);
  writeLine("stdout","Add user accounts defined in file *Path");
}
INPUT *Path="/tempZone/home/rods/testcoll/rodsaccount"
OUTPUT ruleExecOut
