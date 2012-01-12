myTestRule {
#Input parameters are:
#  Recursion flag
#    default
#    recursive  - valid if access level is set to inherit
#  Access Level
#    null
#    read
#    write
#    own
#    inherit
#  User name or group name who will have ACL changed
#  Path or file that will have ACL changed
  msiSetACL("default",*Acl,*User,*Path);
  writeLine("stdout","Set owner access for *User on file *Path");
}
INPUT *User="testuser", *Path="/tempZone/home/rods/sub1/foo3", *Acl = "write"
OUTPUT ruleExecOut
