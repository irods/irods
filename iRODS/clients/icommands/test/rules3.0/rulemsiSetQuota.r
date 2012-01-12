myTestRule {
#Input parameters are:
#  Type of quota (user or group)
#  User or group name
#  Optional resource on which the quota applies (or total for all resources)
#  Quota value in bytes
  msiSetQuota(*Type, *Name, *Resource, *Value);
  writeLine("stdout","Set quota on *Name for resource *Resource to *Value bytes");
}
INPUT *Type="user", *Name="rods", *Resource="demoResc", *Value="1000000000"
OUTPUT ruleExecOut
