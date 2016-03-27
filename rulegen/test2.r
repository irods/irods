myTestRule
{
	acGetIcatResults(*Action,*Condition,*B);
	foreach ( *B )
	{
	  	remote ("andal.sdsc.edu" , "null")
		{
			msiDataObjChksum(*B,*Operation,*C);
		}
		msiGetValByKey(*B,DATA_NAME,*D);
		msiGetValByKey(*B,COLL_NAME,*E);
		writeLine(stdout,"CheckSum of *E/*D is *C");
	}
}

INPUT *Action=chksum,*Condition="COLL_NAME = '/tempZone/home/rods/loopTest'",*Operation=ChksumAll
OUTPUT *Action,*Condition,*Operation,*C,ruleExecOut

