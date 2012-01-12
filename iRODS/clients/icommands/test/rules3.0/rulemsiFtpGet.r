myTestRule {
#Input parameters are:
#  Remote URL
#  New filepath within iRODS
#Output parameter is:
#  Status
  msiFtpGet(*Target, *Destobj, *Status);
  writePosInt("stdout", *Status);
  writeLine("stdout", "");
}
INPUT *Target="ftp://mirror.nyi.net/apache/ant/README.html", *Destobj="/tempZone/home/rods/test/README.html"
OUTPUT ruleExecOut

