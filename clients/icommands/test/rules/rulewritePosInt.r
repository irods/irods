myTestRule {
#Input parameters are:
#  Location (stdout, stderr)
#  Integer
  *A = 1;
  writeLine("stdout","Wrote an integer");
  writePosInt("stdout",*A);
  writeLine("stdout","");
}
INPUT null
OUTPUT ruleExecOut
