myTestRule
{
	for (*I = 0 ; *I < *Count ; *I = *I + 1)
	{
	    	msiXmsgServerConnect(*Conn);
		msiXmsgCreateStream(*Conn,*A,*Tic);
		msiCreateXmsgInp(1,testMsg,1,"TTTest*I",0,"" ,"" ,"" ,*Tic,*MParam);
		msiSendXmsg(*Conn,*MParam);
		msiXmsgServerDisConnect(*Conn);
		remote ("andal.sdsc.edu" , "null")
		{
			msiXmsgServerConnect(*Conn1);
			msiRcvXmsg(*Conn1,*Tic,1,*MType,*MMsg,*MSender);
			writeLine(stdout,*MMsg);
			msiXmsgServerDisConnect(*Conn1);
		}
	}
}
INPUT *A=100 , *Count=10
OUTPUT "",ruleExecOut
