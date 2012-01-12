myTestRule {
#Input parameters are:
#  Message serial number
#  Message type
#    0 SINGLE_MSG_TICKET
#    1 MULTI_MSG_TICKET
#  Number of receivers for message
#  Message body
#  Number of receiving sites, comma separated
#  List of Host Addresses
#  List of Ports
#  Miscellaneous information
#  outXmsgTicketInfoParam from msiXmsgCreateStream
#Output parameter is:
#  Xmsg packet
 for (*I = 0 ; *I < *Count ; *I = *I + 1) 
  { 
    msiXmsgServerConnect(*Conn);
    msiXmsgCreateStream(*Conn,*A,*Tic);
    msiCreateXmsgInp("1","1","1","TTTest*I","0","" ,"" ,"" ,*Tic,*MParam);
    msiSendXmsg(*Conn,*MParam);
    msiXmsgServerDisConnect(*Conn);

    # now read the message that was sent.  The read can be done from a remote server.
    msiXmsgServerConnect(*Conn1);
    msiRcvXmsg(*Conn1,*Tic,"1",*MType,*MMsg,*MSender);
    writeLine("stdout",*MMsg);
    msiXmsgServerDisConnect(*Conn1);
  }
}
INPUT *A=100, *Count=10 
OUTPUT ruleExecOut,*MSender,*Mtype,*MMsg
