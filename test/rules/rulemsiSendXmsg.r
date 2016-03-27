myTestRule {
#Input parameters are:
#  Connection descriptor from msiXmsgServerConnect
#  OutXmsgTicketInfoParam from msiXmsgCreateStream 
#Output from running the example is:
#  TTTest0
#  TTTest1
#  TTTest2
#  TTTest3
  for (*I = 0 ; *I < *Count ; *I = *I + 1) { 
    # send a message
    msiXmsgServerConnect(*Conn);
    msiXmsgCreateStream(*Conn,*A,*Tic);
    msiCreateXmsgInp("1","0","1","TTTest*I","0","" ,"" ,"" ,*Tic,*MParam);
    msiSendXmsg(*Conn,*MParam);
    msiXmsgServerDisConnect(*Conn);

    # now receive the message      
    msiXmsgServerConnect(*Conn1);
    msiRcvXmsg(*Conn1,*Tic,"1",*MType,*MMsg,*MSender);
    writeLine("stdout",*MMsg);
    msiXmsgServerDisConnect(*Conn1);
  }
}
INPUT *A=100, *Count= 4
OUTPUT ruleExecOut
