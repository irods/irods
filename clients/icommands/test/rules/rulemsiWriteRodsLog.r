myTestRule {
#Input parameter is:
#  Message to send to iRODS server log file
#Output parameter is:
#  Status
#Output from running the example is:
#  Message is Test message for irods/server/log/rodsLog
#Output written to log file is:
#  msiWriteRodsLog message: Test message for irods/server/log/rodsLog
   writeLine("stdout","Message is *Message");
   msiWriteRodsLog(*Message,*Status);
}
INPUT *Message="Test message for irods/server/log/rodsLog"
OUTPUT ruleExecOut
