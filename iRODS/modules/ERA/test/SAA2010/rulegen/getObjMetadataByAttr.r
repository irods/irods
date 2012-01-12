########### UNTESTED YET. TO BE COMPLETED ###########
getObjMetadataByAttr(*Object,*Attribute)
{
# Split the path
	msiSplitPath(*Object,*CollName,*DataName);
# Add select field(s)
	msiAddSelectFieldToGenQuery(META_DATA_ATTR_VALUE, null, *GenQInp);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "=", *CollName, *GenQInp);
	msiAddConditionToGenQuery(DATA_NAME, "=", *DataName, *GenQInp);
	msiAddConditionToGenQuery(META_DATA_ATTR_NAME, "=", *Attribute, *GenQInp);
# Run query
	msiExecGenQuery(*GenQInp, *GenQOut);
# Extract results, must be inside loop to work
  	foreach (*GenQOut)
	{
		msiGetValByKey(*GenQOut, "META_DATA_ATTR_VALUE", *MDVal);
	}
# Print out result
#	writeLine(stdout,*MDVal);
}
INPUT *Object = /renci/home/rods/loading/isCollClean.r
OUTPUT ruleExecOut