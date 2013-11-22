myTestRule {
#Input parameter is:
#  Message to send to iRODS server log file
#Output parameter is:
#  Status
#Output from running the example is:
#  Message is Test message for irods/server/log/rodsLog
#Output written to log file is:
#  msiWriteRodsLog message: Test message for irods/server/log/rodsLog
   msiExecCmd("myWorkFlow", *File1, "null","null","null",*Result1);
   msiExecCmd("myWorkFlow", *File2, "null","null","null",*Result2);
   msiGetFormattedSystemTime(*myTime,"human","%d-%d-%d %ldh:%ldm:%lds");
   writeLine("stdout", "Workflow Executed Successfully at *myTime");
}
INPUT 