myTestRule {
#Input parameter is:
#  File containing desired ACLs has format
#    Path-name|user-name|ACL
#    /tempZone/home/rods/test|devtestuser|read
#Output parameter is:
#  Status
  msiLoadACLFromDataObj(*Path,*Status);
  writeLine("stdout","Run ACL modifications loaded from *Path");
}
INPUT *Path="/tempZone/home/rods/test/devtestuser-account-ACL.txt"
OUTPUT ruleExecOut
