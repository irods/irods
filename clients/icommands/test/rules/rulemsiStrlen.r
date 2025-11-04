myTestRule {
#Input parameter is:
#  String
#Output parameter is:
#  Length of string
#Output from running the example is:
#  The String: /tempZone/home/rods/sub1/foo1 has length 29
  msiStrlen(*StringIn,*Length);
  writeLine("stdout","The string: *StringIn has length *Length");
}
INPUT *StringIn="/tempZone/home/rods/sub1/foo1"
OUTPUT ruleExecOut
