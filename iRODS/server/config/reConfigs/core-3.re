acPostProcForPut {ON($objPath like ".*\.mdf") {msiLoadMetadataFromFile ::: msiRollback; } }
acPostProcForPut { }
