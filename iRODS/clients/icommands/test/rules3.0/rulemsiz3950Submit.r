myTestRule {
#Input parameters are:
#  Z30.50 server name
#  Query
#  Syntax for returned record
#Output parameter is:
#  Result
  msiz3950Submit(*Server,*Query,*RecordSyntax,*Out);
  writeBytesBuf("stdout", *Out);
}
INPUT *Server="z3950.loc.gov:7090/Voyager", *Query="@attr 1=1003 Marx", *RecordSyntax="USMARC"
OUTPUT ruleExecOut
