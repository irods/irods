acHasCollMetaAV(*Collection,*Attribute,*Value,*Status)
{
# Add select field(s)
	msiAddSelectFieldToGenQuery(META_COLL_ATTR_NAME, count, *GenQInp);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "=", *Collection, *GenQInp);
	msiAddConditionToGenQuery(META_COLL_ATTR_NAME, "=", *Attribute, *GenQInp);
	msiAddConditionToGenQuery(META_COLL_ATTR_VALUE, "=", *Value, *GenQInp);
# Run query
	msiExecGenQuery(*GenQInp, *GenQOut);
# Extract results, must be inside loop to work
  	foreach (*GenQOut)
	{
		msiGetValByKey(*GenQOut, "META_COLL_ATTR_NAME", *Count);
	}
# Print out result
writeLine(stdout,*Count);
}
INPUT *Collection="/renci/home/saademo/archive_root/another_checksum_series",*Attribute="PolicyDrivenService:SeriesAttributeMarkerAttribute",*Value="requireChecksum"
OUTPUT ruleExecOut