myTestRule {
#Workflow operator to trap an error code of passed command
#Input parameter is:
#  microservice whose error code will be trapped
#Output parameter is:
#  none
  if (errorcode( msiExecCmd(*Cmd, *Arg, "null", "null", "null", *Result)) < 0 ) {
    writeLine("stdout","Microservice execution had an error");
  }
  else { writeLine("stdout","Microservice executed successfully"); }
}
INPUT *Cmd="hello", *ARG="iRODS"
OUTPUT ruleExecOut
