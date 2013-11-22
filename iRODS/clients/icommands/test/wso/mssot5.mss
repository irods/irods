myTestRule {
#Input parameter is:
#  Message to send to iRODS server log file
#Output parameter is:
#  Status
#Output from running the example is:
#  Message is Test message for irods/server/log/rodsLog
#Output written to log file is:
#  msiWriteRodsLog message: Test message for irods/server/log/rodsLog
   msiExecCmd("touch", *File, "null","null","null",*Result);
   writeLine("serverLog", "second test touched file *File");
   writeLine("stdout", "second test touched file *File");
}
INPUT 