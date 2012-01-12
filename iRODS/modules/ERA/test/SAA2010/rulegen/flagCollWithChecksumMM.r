flagCollWithChecksumMM
{
## 1st query to get objects directly in the collection
# Add select field(s)
	msiAddSelectFieldToGenQuery(DATA_ID, COUNT, *GenQInp);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "=", *collection, *GenQInp);
	msiAddConditionToGenQuery(META_DATA_ATTR_NAME, "like", "CHECKSUM_MISMATCH%", *GenQInp);
# Run query
	msiExecGenQuery(*GenQInp, *GenQOut);
# Extract results, must be inside loop to work
  	foreach (*GenQOut)
	{
		msiGetValByKey(*GenQOut, "DATA_ID", *objCount);
	}
	

## 2d query to get objects in subcollections
# Add select field(s)
	msiAddSelectFieldToGenQuery(DATA_ID, COUNT, *GenQInp1);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "like", "*collection/%", *GenQInp1);
	msiAddConditionToGenQuery(META_DATA_ATTR_NAME, "like", "CHECKSUM_MISMATCH%", *GenQInp1);
# Run query
	msiExecGenQuery(*GenQInp1, *GenQOut1);
# Extract results
  	foreach (*GenQOut1)
	{
		msiGetValByKey(*GenQOut1, "DATA_ID", *objCount1);
	}
	
## Add subtotals
	assign(*count, "*objCount + *objCount1" );
	
# Flag coll if found corrupted obj(s)
	if (*count > 0)
	then
	{
#		# Get timestamp
		msiGetSystemTime(*Time, human);
#		# Create new KeyValPair_MS_T
		msiAddKeyVal(*KVP, "CHECKSUM_MISMATCH.*Time", *count);
#		# Add new metadata triplet to collection
		msiAssociateKeyValuePairsToObj(*KVP, *collection, "-C");
	}
	
# Print out corrupted obj count
	writeLine(stdout,*count);
}
INPUT *collection = /tempZone/home/rods
OUTPUT ruleExecOut