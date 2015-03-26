MyTestRule {
#Input parameters are:
#  File name for writing rules
#  Rule structure that will be written
#Output lists the rules in the in-memory rule structure
  msiAdmReadRulesFromFileIntoStruct(*FileName,*Struct);
  msiAdmInsertRulesFromStructIntoDB(*RuleBase,*Struct);
  msiAdmRetrieveRulesFromDBIntoStruct(*RuleBase,"0",*Struct1);
  msiAdmWriteRulesFromStructIntoFile(*FileName1,*Struct1);
  msiAdmAddAppRuleStruct(*FileName1,"","");
  msiAdmShowIRB();
}
INPUT *RuleBase="RajaBase", *FileName="raja", *FileName1="raja1"
OUTPUT ruleExecOut
