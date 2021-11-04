/**
 * @file
 */

/** \mainpage Microservices

\section msiworkflow Rule Engine Workflow Microservices
 - #msiGoodFailure   - Useful when you want to fail but no recovery initiated
 - #msiSleep         - Sleep

\section msicore Core Microservices

 \subsection msiruleengine Rule Engine Microservices
  - #msiAdmShowDVM                              - Displays the currently loaded data variable mappings
  - #msiAdmShowFNM                              - Displays the currently loaded microservices and action (function) name mappings

  - #msiAdmReadDVMapsFromFileIntoStruct         - Reads a DVM configuration file into a DVM structure
  - #msiAdmInsertDVMapsFromStructIntoDB         - Writes a DVM structure into the current DVM base
  - #msiGetDVMapsFromDBIntoStruct               - Populates a DVM structure with DVMs from the given base name
  - #msiAdmWriteDVMapsFromStructIntoFile        - Writes to file the DVMs within a given DVM structure

  - #msiAdmReadFNMapsFromFileIntoStruct         - Reads a FNM configuration file into a FNM structure
  - #msiAdmInsertFNMapsFromStructIntoDB         - Writes an FNM structure into the current FNM base
  - #msiGetFNMapsFromDBIntoStruct               - Populates an FNM structure with FNMs from the given base name
  - #msiAdmWriteFNMapsFromStructIntoFile        - Writes to file the FNMs within a given FNM structure

  - #msiAdmReadMSrvcsFromFileIntoStruct         - Reads a microservice configuration file into a microservice structure
  - #msiAdmInsertMSrvcsFromStructIntoDB         - Writes a microservice structure into the current microservices base
  - #msiGetMSrvcsFromDBIntoStruct               - Populates a microservice structure with microservices from the given base name
  - #msiAdmWriteMSrvcsFromStructIntoFile        - Writes to file the microservices within a given microservice structure

 \subsection msihelper Helper Microservices
  Can be called by client through irule.
  - #msiGetStdoutInExecCmdOut - Gets stdout buffer from ExecCmdOut into buffer
  - #msiGetStderrInExecCmdOut - Gets stderr buffer from ExecCmdOut into buffer
  - #msiWriteRodsLog - Writes a message into the server rodsLog
  - #msiAddKeyValToMspStr - Adds a key and value to existing msKeyValStr
  - #msiSplitPath - Splits a pathname into parent and child values
  - #msiGetSessionVarValue - Gets the value of a session variable in the rei
  - #msiExit - Add a user message to the error stack

 \subsection mainmsilowlevel Data Object Low-level Microservices
  Can be called by client through irule.
  - #msiDataObjCreate - Create a data object
  - #msiDataObjOpen - Open a data object
  - #msiDataObjClose - Close an opened data object
  - #msiDataObjLseek - Repositions the read/write offset of an open data object
  - #msiDataObjRead - Read an opened data object
  - #msiDataObjWrite - Write to an opened data object

 \subsection msidataobject Data Object Microservices
  Can be called by client through irule.
  - #msiDataObjUnlink - Delete a data object
  - #msiDataObjRepl - Replicate a data object
  - #msiDataObjCopy - Copy a data object
  - #msiDataObjGet - Get a data object
  - #msiDataObjPut - Put a data object
  - #msiDataObjChksum - Checksum a data object
  - #msiDataObjPhymv - Move a data object from one resource to another
  - #msiDataObjRename - Rename a data object
  - #msiDataObjTrim - Trim the replicas of a data object
  - #msiPhyPathReg - Register a physical file into iRODS
  - #msiObjStat - Stat an iRODS object
  - #msiDataObjRsync - Syncs a data object from a source to a destination
  - #msiCollRsync - Recursively syncs a source collection to a target collection
  - #msiGetObjType - Finds if a given value is a data, coll, resc, ...
  - #msiCheckPermission - Check if a data object permission is the same as the one given
  - #msiCheckOwner - Check if the user is the owner of the data object
  - #msiSetReplComment - Sets the data_comments attribute of a data object

 \subsection msicollection Collection Microservices
  - #msiCollCreate - Create a collection
  - #msiCollRepl - Replicate all files in a collection
  - #msiRmColl - Delete a collection
  - #msiTarFileExtract - Extracts a tar object file into a target collection
  - #msiTarFileCreate - Creates a tar object file from a target collection
  - #msiPhyBundleColl - Bundles a collection into a number of tar files, similar to the iphybun command

 \subsection msiproxy Proxy Command Microservices
  - #msiExecCmd - Remotely execute a command

 \subsection msiicat iCAT Microservices
 iCAT System Services
  - #msiQuota - Calculates storage usage and sets quota values
  - #msiCommit  - Commits the database transaction
  - #msiRollback - Rollback the current database transaction
  - #msiSetACL - Changes the ACL for a given pathname, either a collection or a data object
  - #msiCreateUser - Creates a new user
  - #msiDeleteUser - Deletes a user
  - #msiAddUserToGroup - Adds a user to a group
  - #msiCreateCollByAdmin  - Creates a collection by administrator
  - #msiDeleteCollByAdmin  - Deletes a collection by administrator
  - #msiRenameLocalZone  - Renames the local zone by updating various tables
  - #msiRenameCollection  - Renames a collection; used via a rule with the above #msiRenameLocalZone
  - #msiRenameLocalZoneCollection  - Renames the local zone collection
  - #msiExecStrCondQuery - Creates an iCAT query, given a condition string, and executes it and returns the values
  - #msiExecGenQuery - Executes a given general query structure and returns results
  - #msiMakeQuery - Creates a psuedo-SQL query, given a select list and a condition list
  - #msiGetMoreRows - Continues an unfinished query and #msiExecStrCondQuery and returns results
  - #msiCloseGenQuery - Closes an unfinished query.  Based on #msiGetMoreRows.
  - #msiMakeGenQuery  - A combination of #msiMakeQuery and #msiExecGenQuery and returns the results of the execution
  - #msiGetContInxFromGenQueryOut - Gets continue index value generated by #msiExecGenQuery
  - #msiAddSelectFieldToGenQuery - Sets a select field in a genQueryInp_t
  - #msiPrintGenQueryInp - Prints the given GenQueryInp_MS_T to the given target buffer
  - #msiAddConditionToGenQuery - Adds a condition to a genQueryInp_t
  - #msiPrintGenQueryOutToBuffer - Writes the contents of a GenQueryOut_MS_T into a BUF_LEN_MS_T
  - #msiDeleteUnusedAVUs - Deletes unused AVUs from the iCAT

 \subsection msistring String Manipulation Microservices
  - #msiStrlen   - Returns the length of a given string
  - #msiStrchop  - Removes the last character of a given string
  - #msiSubstr   - Returns a substring of the given string

 \subsection msiemail Email Microservices
  - #msiSendMail   - Sends email

 \subsection msikv Key-Value (Attr-Value) Microservices
  - #writeKeyValPairs - Writes key-value pairs to stdout or stderr and with given separator
  - #msiPrintKeyValPair - Prints key-value pairs to rei's stdout separated by =
  - #msiGetValByKey  - Extracts the corresponding value, given a key and a keyValPair struct
  - #msiString2KeyValPair - Converts a \%-separated key=value pair strings into keyValPair structure
  - #msiStrArray2String - Converts an array of strings to a string separated by \%-signs
  - #msiAssociateKeyValuePairsToObj  - Ingests object metadata into iCAT from a AVU structure
  - #msiRemoveKeyValuePairsFromObj  - Removes object metadata from iCAT using a AVU structure
  - #msiSetKeyValuePairsToObj - Ingests or overwrites object metadata into iCAT from a AVU structure
  - #msiAddKeyVal - Adds a new key and value to a keyValPair_t

 \subsection msiotheruser Other User Microservices
  - #msiModAVUMetadata - Modifies AVU metadata on a user, resource, data object, or collection
  - #msi_atomic_apply_acl_operations - Atomically modifies multiple ACLs on a single data object or collection
  - #msi_atomic_apply_metadata_operations - Atomically modifies multiple AVUs on a single user, resource, data object, or collection
  - #msi_get_agent_pid - Gets the pid of the agent which executes this microservice
  - #msi_touch - Changes the mtime of a data object or collection
  - #msiExtractNaraMetadata - Extracts NARA style metadata from a local configuration file
  - #msiApplyDCMetadataTemplate - Adds Dublin Core Metadata fields to an object or collection
  - #msiRegisterData - Registers a new data object
  - #writeBytesBuf - Writes the buffer in an inOutStruct to stdout or stderr
  - #msiBytesBufToStr - Converts a bytesBuf_t to a string
  - #msiStrToBytesBuf - Converts a string to a bytesBuf_t
  - #msiFreeBuffer - Frees a buffer in an inOutStruct, or stdout or stderr
  - #writePosInt  - Writes an integer to stdout or stderr
  - #msiGetDiffTime - Returns the difference between two system timestamps given in unix format (stored in string)
  - #msiGetSystemTime - Returns the local system time of server
  - #msiGetFormattedSystemTime - Returns the local system time, formatted
  - #msiHumanToSystemTime - Converts a human readable date to a system timestamp
  - #msiGetIcatTime - Returns the system time for the iCAT server
  - #msiGetTaggedValueFromString  - Gets the value from a file in tagged-format (psuedo-XML), given a tag-name
  - #msiExtractTemplateMDFromBuf     - Extracts AVU info using template
  - #msiReadMDTemplateIntoTagStruct  - Loads template file contents into tag structure

 \subsection msisystem System Microservices
  Can only be called by the server process
  - #msiSetDefaultResc - Sets the default resource
  - #msiSetNoDirectRescInp - Sets a list of resources that cannot be used by a normal user directly
  - #msiSetDataObjPreferredResc - Specifies the preferred copy to use, if the data has multiple copies
  - #msiSetDataObjAvoidResc - Specifies the copy to avoid
  - #msiSetGraftPathScheme - Sets the scheme for composing the physical path in the vault to GRAFT_PATH
  - #msiSetRandomScheme - Sets the the scheme for composing the physical path in the vault to RANDOM
  - #msiSetResource  - sets the resource from default
  - #msiSortDataObj - Sort the replica randomly when choosing which copy to use
  - #msiSetNumThreads - specify the parameters for determining the number of threads to use for data transfer
  - #msiSysChksumDataObj - Checksums a data object
  - #msiSysReplDataObj - Replicates a data object
  - #msiSysMetaModify - Modifies system metadata
  - #msiStageDataObj - Stages the data object to the specified resource before operation
  - #msiNoChkFilePathPerm - Does not check file path permission when registering a file
  - #msiSetChkFilePathPerm - Sets the check type for file path permission check when registering a file
  - #msiNoTrashCan - Sets the policy to no trash can
  - #msiSetPublicUserOpr - Sets a list of operations that can be performed by the user "public"
  - #msiCheckHostAccessControl - Sets the access control policy
  - #msiServerMonPerf - Monitors the servers' activity and performance
  - #msiFlushMonStat - Flushes the servers' monitoring statistics
  - #msiDigestMonStat - Calculates and stores a digest performance value for each connected resource
  - #msiDeleteDisallowed - Sets the policy for determining certain data cannot be deleted
  - #msiSetDataTypeFromExt - Gets the data type based on file name extension
  - #msiSetReServerNumProc - Sets the number of processes for the rule engine server
  - #msiSetRescQuotaPolicy - Sets the resource quota to on or off
  - #msiListEnabledMS - Returns the list of compiled microservices on the local iRODS server
  - #msiSetBulkPutPostProcPolicy - Sets whether acPostProcForPut should be run after a bulk put
  - #msisync_to_archive - Manually replicates a dataObject from compound cache to archive

 \section msiadmin Admin Microservices
  Can only be called by an administrator
  - #msiSetQuota - Sets disk usage quota for a user or group

**/
