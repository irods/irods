myTestRule {
# Input parameters are:
#  Collection that is being replicated
#  Recursion flag, set to "true" if enabled
#  Required number of replicas
#  Resource group that contains multiple resources
#  e-mail account which will be notified of success
# Output from execution of the rule is:
#  Verify the checksum and number of replicas (1) for collection /tempZone/home/rods/ruletest
#  E-mail is sent to irod-chat@googlegroups.com
  msiAutoReplicateService(*COLL, "true", *NUMREPLICA, *RESOURCEGROUP, *EMAIL);
  writeLine("stdout","Verify the checksum and number of replicas (*NUMREPLICA) for collection *COLL");
  writeLine("stdout","E-mail is sent to *EMAIL");
}
INPUT *COLL="/tempZone/home/rods/ruletest", *NUMREPLICA="1", *RESOURCEGROUP="testgroup", *EMAIL="irod-chat@googlegroups.com"
OUTPUT ruleExecOut
