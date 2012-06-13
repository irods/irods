myTestRule ()
{
    if (msiGetRescAddr(*resource,*address) >= 0) {
	writeLine ("stdout", "address of resource: *resource is *address");
    } else {
        writeLine ("stdout", "msiGetRescAddr error");
    }
}
INPUT *resource="demoResc"
OUTPUT ruleExecOut

