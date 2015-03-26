myTestRule {
#Input parameters are:
#  Status error to add to the error stack
#  Message to add to the error stack
#Output from running the example is:
#  Error number 200 and message Test Error
  writeLine("stdout","Error number *Error and message *Message");
  msiExit(*Error,*Message);
}
INPUT *Error="200", *Message="Test Error"
OUTPUT ruleExecOut
