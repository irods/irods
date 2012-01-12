myTestRule {
#Input parameters are:
#  Data Object path
#  Optional flags in form keyword=value
#    destRescName - the target resource for the replica
#    backupRescName - specifies use of the resource for the backup mode
#    rescName - the resource holding the source data
#    updateRepl= - specifies all replicas will be updated
#    replNum - specifies the replica number to use as the source
#    numThreads - specifies the number of threads to use for transmission
#    all - specifies to replicate to all resources in a resource group
#    irodsAdmin - enables administrator to replicate other users' files
#    verifyChksum - verify the transfer using checksums
#    rbudpTransfer - use Reliable Blast UDP for transport
#    rbudpSendRate - the transmission rate in kbits/sec, default is 600 kbits/sec
#    rbudpPackSize - the packet size in bytes, default is 8192
#Output parameter is:
#  Status
#Output from running the example is:
#  The file /tempZone/home/rods/sub1/foo3 is replicated onto resource testResc
  msiDataObjRepl(*SourceFile,"destRescName=*Resource",*Status);
  writeLine("stdout","The file *SourceFile is replicated onto resource *Resource");
}
INPUT *SourceFile="/tempZone/home/rods/sub1/foo3", *Resource="testResc" 
OUTPUT ruleExecOut
