myTestRule {
#Input parameters are:
#  Name of object
#  Access permission that will be checked
#Output parameter is:
#  Result, 0 for failure and 1 for success
  msiCheckAccess(*Path,*Acl,*Result);
  if(*Result == 0) {
    writeLine("stdout","File *Path does not have access *Acl"); }
  else {writeLine("stdout","File *Path has access *Acl"); }
}
INPUT *Path = "/tempZone/home/rods/sub1/foo1", *Acl = "own"
OUTPUT ruleExecOut 
