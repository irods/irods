MyTestRule {
#Input Parameter is:
#  File containing rules
#Output parameter is:
#  Buffer holding rule struct
  msiAdmReadRulesFromFileIntoStruct(*FileName,*Struct);
  msiAdmInsertRulesFromStructIntoDB(*RuleBase,*Struct);
  msiAdmRetrieveRulesFromDBIntoStruct(*RuleBase,"0",*Struct1);
  msiAdmWriteRulesFromStructIntoFile(*FileName1,*Struct1);
  msiAdmAddAppRuleStruct(*FileName1,"","");
  msiAdmShowIRB();
}
INPUT *RuleBase="RajaBase",*FileName="raja",*FileName1="raja1"
OUTPUT ruleExecOut
