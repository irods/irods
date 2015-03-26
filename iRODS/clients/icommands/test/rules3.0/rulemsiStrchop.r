myTestRule {
#Input parameter is:
#  String
#Output parameter is:
#  String without the last character
#Output from running the example is:
#  The input string is:  /tempZone/home/rods/sub1/foo1/
#  The output string is: /tempZone/home/rods/sub1/foo1
  msiStrchop(*StringIn,*StringOut);
  writeLine("stdout","The input string is:  *StringIn");
  writeLine("stdout","The output string is: *StringOut");
}
INPUT *StringIn="/tempZone/home/rods/sub1/foo1/"
OUTPUT ruleExecOut
