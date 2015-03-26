myTestRule {
#Input parameters are:
#  Session variable
#    Session variable without the $ sign
#    all      - output all of the defined variables
#  Output mode flag:
#    server   - log the output to the server log
#    client   - send the output to the client in rError
#    all      - send the output to both client and server
#Output from running the example is:
#  Variables are written to the log file
#Output in irods/server/log/rodsLog.2011.6.1 log file is:
#  msiGetSessionVarValue: userNameClient=rods
   msiGetSessionVarValue(*A,"server");
   writeLine("stdout","Variables are written to the log file");
}
INPUT *A="userNameClient"
OUTPUT ruleExecOut
