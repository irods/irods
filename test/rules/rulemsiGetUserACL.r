myTestRule {
#Input parameter is:
#  User name
#Output parameters are:
#  Buffer holding ACLs
#  Status
  msiGetUserACL(*User,*Buf,*Status);
  writeBytesBuf("stdout",*Buf);
}
INPUT *User="rods"
OUTPUT ruleExecOut
