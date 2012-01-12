myTestRule {
#Input parameters are:
#  Data object path
#  Optional destination resource
#  Optional physical path to register
#  Optional flag for type of
#    collection  - specifies the path is a directory
#    null        - specifies the path is a file
#    mountPoint  - specifies to mount the physical path
#    linkPoint   - specifies soft link the physical path
#Output parameter is:
#  Status
#Output from running the example is:
#  The local collection /home/reagan/irods-scripts/ruletest is mounted under the logical collection /tempZone/home/rods/irods-rules
   msiPhyPathReg(*DestCollection,*Resource,*SourceDirectory,"mountPoint",*Stat);
   writeLine("stdout","The local collection *SourceDirectory is mounted under the logical collection *DestCollection");
}
INPUT *DestCollection="/tempZone/home/rods/irods-rules", *SourceDirectory="/home/reagan/irods-scripts/ruletest", *Resource="demoResc"
OUTPUT ruleExecOut
