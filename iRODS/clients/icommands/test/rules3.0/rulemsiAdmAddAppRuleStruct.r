myTestRule {
# Examples are in irods/server/config/reConfigs
# Input parameters are:
#   Rule file without the .re extension
#   Session variable file name mapping file without the .dvm extension
#   Application microservice mapping file without the .fnm extension
# Output from running the example is:
#   List of the rules in the In-memory Rule Base
  msiAdmAddAppRuleStruct("*File","","");
  msiAdmShowIRB();
}
INPUT *File="core3"
OUTPUT ruleExecOut
