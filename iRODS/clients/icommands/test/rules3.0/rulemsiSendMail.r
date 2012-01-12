myTestRule {
#Input parameters are:
#  Address 
#  Subject of e-mail
#  Message body
  msiSendMail(*Address,*Subject,*Body);
  writeLine("stdout","Sent e-mail to *Address about *Subject");
}
INPUT *Address="irod-chat@googlegroups.com",*Subject="Test message",*Body="Testing the msiSendMail microservice"
OUTPUT ruleExecOut
