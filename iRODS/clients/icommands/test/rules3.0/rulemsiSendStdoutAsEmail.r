myTestRule {
#Input parameters are:
#  Address
#  Subject
  writeLine("stdout","Message from stdout buffer");
  msiSendStdoutAsEmail(*Address,*Subject);
  writeLine("stdout","Sent e-mail to *Address about *Subject");
}
INPUT *Address="irod-chat@googlegroups.com", *Subject="Test message"
OUTPUT ruleExecOut
