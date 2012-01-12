myTestRule {
#Input parameter is:
#  File containing desired ACLs has format
#    Path-name|user-name|ACL
#    /tempZone/home/rods/rodsaccount|guest|read
#Output parameter is:
#  Status
  msiLoadACLFromDataObj(*Path,*Status);
  writeLine("stdout","Add ACLs to files from an input file");
}
INPUT *Path="/tempZone/home/rods/testcoll/rodsaccountACL"
OUTPUT ruleExecOut
