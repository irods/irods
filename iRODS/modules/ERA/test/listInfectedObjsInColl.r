ListInfectedObjsInColl
## Doesn't like CAT_NO_ROWS_FOUND errors in partial queries.
{
## 1st query to get objects directly in the collection
# Add select field(s)
	msiAddSelectFieldToGenQuery(DATA_NAME, null, *GenQInp);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "=", *collection, *GenQInp);
	msiAddConditionToGenQuery(META_DATA_ATTR_NAME, "like", "INFECTED%", *GenQInp);
# Run query
	msiExecGenQuery(*GenQInp, *GenQOut);
# Extract and print out results, must be inside loop to work
  	foreach (*GenQOut)
	{
		msiGetValByKey(*GenQOut, "DATA_NAME", *dataName);
		writeLine(stdout, "*collection/*dataName");
	}
	

## 2d query to get objects in subcollections
# Add select field(s)
	msiAddSelectFieldToGenQuery(DATA_NAME, null, *GenQInp1);
	msiAddSelectFieldToGenQuery(COLL_NAME, null, *GenQInp1);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "like", "*collection/%", *GenQInp1);
	msiAddConditionToGenQuery(META_DATA_ATTR_NAME, "like", "INFECTED%", *GenQInp1);
# Run query
	msiExecGenQuery(*GenQInp1, *GenQOut1);
# Extract and print out results
  	foreach (*GenQOut1)
	{
		msiGetValByKey(*GenQOut1, "DATA_NAME", *dataName);
		msiGetValByKey(*GenQOut1, "COLL_NAME", *collName);
		writeLine(stdout, "*collName/*dataName");
	}

}
INPUT *collection = /tempZone/home/rods
OUTPUT ruleExecOut