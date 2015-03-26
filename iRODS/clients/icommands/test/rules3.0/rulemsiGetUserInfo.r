myTestRule {
#Input parameter is:
#  User
#Output parameters are:
#  Buffer listing the user account information
#  Status
  msiGetUserInfo(*User,*Buf,*Status);
  writeBytesBuf("stdout",*Buf);
}
INPUT *User="rods"
OUTPUT ruleExecOut
