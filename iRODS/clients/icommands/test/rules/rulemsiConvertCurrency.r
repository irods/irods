myTestRule {
#Input parameters are:
#  Country code for original currency - 3 letters
#  Country code for desired currency - 3 letters
#Output parameter is:
#  Conversion rate
  msiConvertCurrency(*InCurr,*OutCurr,*Rate);
  writeLine("stdout", "Conversion rate is *Rate");
}
INPUT *InCurr="USA", *OutCurr="UK"
OUTPUT ruleExecOut
