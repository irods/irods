myTestRule {
#Input parameters are:
#  String
#  Offset from start counting from 0.  If negative, count from end
#  Length of the substring
#Output parameter is:
#  Substring
#Output from running the example is:
#  The input string is:  /tempZone/home/rods/sub1/foo1/
#  The offset is 10 and the length is 4
#  The output string is: home
  msiSubstr(*StringIn,*Offset,*Length,*StringOut);
  writeLine("stdout","The input string is:  *StringIn");
  writeLine("stdout","The offset is *Offset and the length is *Length");
  writeLine("stdout","The output string is: *StringOut");
}
INPUT *StringIn="/tempZone/home/rods/sub1/foo1/", *Offset="10", *Length="4"
OUTPUT ruleExecOut
