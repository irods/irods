MyTestRule {
#Input parameters are:
#  Name of rule base
#  Version of the rule base
#Output parameter is:
#  Buffer to hold rules
#Output from running the example is a list of the rules
  msiAdmReadRulesFromFileIntoStruct(*FileName,*Struct);
  msiAdmInsertRulesFromStructIntoDB(*RuleBase,*Struct);
  msiAdmRetrieveRulesFromDBIntoStruct(*RuleBase,"0",*Struct1);
  msiAdmWriteRulesFromStructIntoFile(*FileName1,*Struct1);
  msiAdmAddAppRuleStruct(*FileName1,"","");
  msiAdmShowIRB();
}
INPUT *RuleBase="RajaBase", *FileName="raja", *FileName1="raja1"
OUTPUT ruleExecOut
