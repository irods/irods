applyDCMetadataTemplate {
	msiApplyDCMetadataTemplate(*obj, *status);
	writePosInt("stdout",*status);
	writeLine("stdout","");
}
INPUT *obj="/pho27/home/rods/LICENSE.txt"
OUTPUT ruleExecOut