myTestRule {
#Input parameter is:
#  Options - currently none are specified for controlling server backup
#Output parameter is:
#  Result - a keyvalpair structure holding number of files and size
#
#  This will take a while to run.
#  Backup files are stored in a directory as hostname_timestamp:
#
#  $ ils system_backups
#  /tempZone/home/rods/system_backups:
#    C- /tempZone/home/rods/system_backups/localhost_2011-08-19.16:00:29
#
#
  msiServerBackup(*Opt,*Result);
  writeKeyValPairs("stdout",*Result, " : ");
}
INPUT *Opt=""
OUTPUT ruleExecOut
