isCollClean
{
## 1st query to get objects directly in the collection
# Add select field(s)
	msiAddSelectFieldToGenQuery(DATA_ID, COUNT, *GenQInp);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "=", *collection, *GenQInp);
	msiAddConditionToGenQuery(META_DATA_ATTR_NAME, "like", "INFECTED%", *GenQInp);
# Run query
	msiExecGenQuery(*GenQInp, *GenQOut);
# Extract and print out results, must be inside loop to work
  	foreach (*GenQOut)
	{
		msiGetValByKey(*GenQOut, "DATA_ID", *objCount);
	}
	

## 2d query to get objects in subcollections
# Add select field(s)
	msiAddSelectFieldToGenQuery(DATA_ID, COUNT, *GenQInp1);
# Add condition(s)
	msiAddConditionToGenQuery(COLL_NAME, "like", "*collection/%", *GenQInp1);
	msiAddConditionToGenQuery(META_DATA_ATTR_NAME, "like", "INFECTED%", *GenQInp1);
# Run query
	msiExecGenQuery(*GenQInp1, *GenQOut1);
# Extract and print out results
  	foreach (*GenQOut1)
	{
		msiGetValByKey(*GenQOut1, "DATA_ID", *objCount1);
	}
	
## Add and print out total
	assign(*count, "*objCount + *objCount1" );  
	writeLine(stdout,*count);

}
INPUT *collection = /tempZone/home/rods
OUTPUT ruleExecOut