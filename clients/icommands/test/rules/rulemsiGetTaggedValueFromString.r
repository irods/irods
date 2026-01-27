myTestRule {
#Input parameters are:
#  Tag string
#  Input string
#Output parameter is:
#  Value associated with tag
  writeLine("stdout","String that is tested is");
  writeLine("stdout","*Str");
  msiGetTaggedValueFromString(*Tag,*Str,*Val);
  writeLine("stdout","Found value is *Val");
}
INPUT *Tag="Mail", *Str="<Mail>IP-address</Mail>"
OUTPUT ruleExecOut
