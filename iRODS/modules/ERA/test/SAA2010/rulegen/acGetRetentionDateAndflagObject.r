acGetRetentionDateAndFlagObj(*Collection,*Object)
{
# Add select field(s)
	msiAddSelectFieldToGenQuery(META_COLL_ATTR_UNITS, null, *GenQInp);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "=", *Collection, *GenQInp);
	msiAddConditionToGenQuery(META_COLL_ATTR_NAME, "=", "PolicyDrivenService:SeriesAttributeMarkerAttribute", *GenQInp);
	msiAddConditionToGenQuery(META_COLL_ATTR_VALUE, "=", "retentionDays", *GenQInp);
# Run query
	msiExecGenQuery(*GenQInp, *GenQOut);
# Extract results, must be inside loop to work
  	foreach (*GenQOut)
	{
		msiGetValByKey(*GenQOut, "META_COLL_ATTR_UNITS", *RD);
	}
# Create new KeyValPair_MS_T
	msiAddKeyVal(*KVP, "RETENTION_DATE", *RD);
# Add new metadata triplet to object
	msiAssociateKeyValuePairsToObj(*KVP, *Object, "-d");
}
INPUT *Collection="/renci/home/saademo/archive_root/retention_date_and_vscan",*Object="/renci/home/saademo/archive_root/retention_date_and_vscan/listInfectedObjsInColl.r"
OUTPUT ruleExecOut