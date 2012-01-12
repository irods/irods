myTestRule {
# Input parameters are:
#  Database resource name
# Output from running the example is:
#  Changes to database resource dbr2 were rolled back
  msiDbrRollback(*DBR);
  writeLine("stdout","Changes to database resource *DBR were rolled back");
}
INPUT *DBR="dbr2"
OUTPUT ruleExecOut
