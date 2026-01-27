myTestRule {
# Only executables stored within irods/server/bin/cmd can be run
#Input parameter is:
#  Output buffer from the exec command which holds the status, output, and error messages
#Output parameter is:
#  Buffer to hold the retrieved error message
#Output from running the example is:
#  Error message is
  
  msiExecCmd(*Cmd,*ARG,"","","",*HELLO_OUT);
  
  # *HELLO_OUT holds the status, output and error messages
  msiGetStderrInExecCmdOut(*HELLO_OUT,*ErrorOut);
  writeLine("stdout","Error message is *ErrorOut");
}
INPUT *Cmd="hello", *ARG="iRODS"
OUTPUT ruleExecOut
