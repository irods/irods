GuessAndSetDataType {
	msiGuessDataType(*objPath,*dataType,*Status);
	msiSetDataType("",*objPath,*dataType,*Status);
	writeLine("stdout","data_type for *objPath set as *dataType");
}
INPUT *objPath="/pho27/home/rods/clim_change_scenario.pdf"
OUTPUT ruleExecOut