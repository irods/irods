getCollectionSize {
	msiGetCollectionSize(*collection, *KVPairs, *status);
	writeKeyValPairs("stdout", *KVPairs, ": ");
}
INPUT *collection="/pho27/home/rods/test"
OUTPUT ruleExecOut