myTestRule {
#Input parameters are:
#  File
#  Current collection path
#  Master collection path
#  Note that the directory structures within the current collection are mirrored into the master
#  Merges one file at a time, if file already exists creates a replica
#  The microservice uses two commands in iRODS/server/bin/cmd:
#    ln    - softlink
#    mkdir - make directory
#Output parameter is:
#  Status
#Output from running example is:
#  List of files in the master directory before the merge
#  List of files in the master directory after the merge
  msiSplitPath(*Path,*Coll,*Fileorig);
  writeLine("stdout","Collection *Master has initial files");
  msiExecStrCondQuery("SELECT DATA_NAME,DATA_ID where COLL_NAME = '*Master'",*QOut);
  foreach (*QOut) {
    msiGetValByKey(*QOut,"DATA_NAME",*File);
    writeLine("stdout",*File);
  }
  msiMergeDataCopies(*Path,*Coll,*Master,*Status);
  writeLine("stdout","Collection *Master has files after merge");
  msiExecStrCondQuery("SELECT DATA_NAME,DATA_ID where COLL_NAME = '*Master'",*QOut);
  foreach (*QOut) {
    msiGetValByKey(*QOut,"DATA_NAME",*File);
    writeLine("stdout",*File);
  }
}
INPUT *Path="/tempZone/home/rods/sub1/foo1", *Master="/tempZone/home/rods/test"
OUTPUT ruleExecOut
