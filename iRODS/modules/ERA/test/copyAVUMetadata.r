copyAVUMetadata {
	msiStripAVUs(*destObj,"null",*junk);
	msiCopyAVUMetadata(*srcObj,*destObj,*status);
	writePosInt("stdout",*status);
	writeLine("stdout","");
}
INPUT *srcObj="/pho27/home/rods/foo1.txt",*destObj="/pho27/home/rods/foo2.txt"
OUTPUT ruleExecOut