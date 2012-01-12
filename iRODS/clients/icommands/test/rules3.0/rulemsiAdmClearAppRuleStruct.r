myTestRule {
#No Input parameter
# Output from running the example:
#  List of rules after adding rule and after clearing rules
  msiAdmAddAppRuleStruct(*A,"","");
  msiAdmShowIRB();
  msiAdmClearAppRuleStruct;
  msiAdmShowIRB();
}
INPUT *A="nara"
OUTPUT ruleExecOut
