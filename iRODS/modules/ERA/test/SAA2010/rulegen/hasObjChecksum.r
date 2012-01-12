hasObjChecksum
{
# Split the path
	msiSplitPath(*Object,*CollName,*DataName);
# Add select field(s)
	msiAddSelectFieldToGenQuery(DATA_CHECKSUM, COUNT, *GenQInp);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "=", *CollName, *GenQInp);
	msiAddConditionToGenQuery(DATA_NAME, "=", *DataName, *GenQInp);
# Run query
	msiExecGenQuery(*GenQInp, *GenQOut);
# Extract results, must be inside loop to work
  	foreach (*GenQOut)
	{
		msiGetValByKey(*GenQOut, "DATA_CHECKSUM", *count);
	}
		
# Flag obj if count == 0
	if (*count > 0)
	then
	{
#		# Get timestamp
		msiGetSystemTime(*Time, human);
#		# Create new KeyValPair_MS_T
		msiAddKeyVal(*KVP, "CHECKSUM_REGISTERED.*Time", *count);
#		# Add new metadata triplet to object
		msiAssociateKeyValuePairsToObj(*KVP, *Object, "-d");
	}
	else
	{
#		# Get timestamp
		msiGetSystemTime(*Time, human);
#		# Create new KeyValPair_MS_T
		msiAddKeyVal(*KVP, "CHECKSUM_MISSING.*Time", *count);
#		# Add new metadata triplet to object
		msiAssociateKeyValuePairsToObj(*KVP, *Object, "-d");
	}
	
# Print out infected obj count
#	writeLine(stdout,*count);
}
INPUT *Object = /renci/home/rods/loading/isCollClean.r
OUTPUT ruleExecOut