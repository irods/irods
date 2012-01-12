collectionSpiderExample {
# Lists the object IDs of all objects in a collection and its subcollections
	*actions=``{
					msiIsData(*objects, *dataID, *foo); 
					writePosInt('stdout', *dataID); 
					writeLine('stdout', '');
			}``;
	msiCollectionSpider(*collection, *objects, *actions, *status);
}
INPUT *collection="/pho27/home/rods/test"
OUTPUT ruleExecOut