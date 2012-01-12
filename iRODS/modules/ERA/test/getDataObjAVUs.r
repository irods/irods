getDataObjAVUs {
	msiGetDataObjAVUs(*obj,*buf);
	writeBytesBuf("stdout",*buf);
}
INPUT *obj="/pho27/home/rods/LICENSE.txt"
OUTPUT ruleExecOut