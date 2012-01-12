acScanFileAndFlagObject(*ObjPath,*FilePath,*Resource)
{
# Run clamscan script on resource
	msiExecCmd("scanfile.py", *FilePath, *Resource, null, null, *CmdOut);
# Save operation status
	assign(*Status, $status);
# Get stdout from last call
	msiGetStdoutInExecCmdOut(*CmdOut, *StdoutStr);

# Passed, failed, or error?
	if (*Status == 0)
	then
#	# Scan successful, object passed
	{
#		# Get timestamp
		msiGetSystemTime(*Time, human);
#		# Create new KeyValPair_MS_T
		msiAddKeyVal(*KVP, "VIRUS_SCAN_PASSED.*Time", *StdoutStr);
#		# Add new metadata triplet to object
		msiAssociateKeyValuePairsToObj(*KVP, *ObjPath, "-d");
	}
	else
	{
		if (*Status == 344000)
		then
#		# Scan successful, object failed
		{
#			# Get timestamp
			msiGetSystemTime(*Time, human);
#			# Create new KeyValPair_MS_T
			msiAddKeyVal(*KVP, "VIRUS_SCAN_FAILED.*Time", *StdoutStr);
#			# Add new metadata triplet to object
			msiAssociateKeyValuePairsToObj(*KVP, *ObjPath, "-d");
		}
		else
#		# Scan failed (command execution error)
		{
			nop;
		}
	}

}
INPUT *foo = ""
OUTPUT ruleExecOut