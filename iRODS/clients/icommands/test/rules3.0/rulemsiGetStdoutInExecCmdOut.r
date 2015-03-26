myTestRule {
#Input parameter is:
#  Buffer holding the status, output and error messages from the command execution
#Output parameter is:
#  Buffer holding the output message
#Output from executing the command is
#  Output message is Hello World iRODS from irods
  msiExecCmd("hello",*ARG,"","","",*HELLO_OUT);

  # *HELLO_OUT holds the status, output and error messages
  msiGetStdoutInExecCmdOut(*HELLO_OUT,*Out);
  writeLine("stdout","Output message is *Out");
}
INPUT *ARG="iRODS"
OUTPUT ruleExecOut
