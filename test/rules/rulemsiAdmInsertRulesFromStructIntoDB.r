MyTestRule {
#Input parameters are:
#  Name of base that is being added
#  Buffer containing rules
#Output from running the example:
#  List of rules in the In-memory Rule Base
  msiAdmReadRulesFromFileIntoStruct(*FileName,*Struct);
  msiAdmInsertRulesFromStructIntoDB(*RuleBase,*Struct);
  msiAdmRetrieveRulesFromDBIntoStruct(*RuleBase,"0",*Struct1);
  msiAdmWriteRulesFromStructIntoFile(*FileName1,*Struct1);
  msiAdmAddAppRuleStruct(*FileName1,"","");
  msiAdmShowIRB();
}
INPUT *RuleBase="RajaBase",*FileName="raja",*FileName1="raja1"
OUTPUT ruleExecOut
