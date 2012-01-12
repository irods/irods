myTestRule {
#Input parameters are:
#  Stream ID 
#  Header
#  Message
#Output from running the example is:
#  Sequence Number is 0
#  Message Header is Test
#  Message is Body of the message
#  Sender is rods@tempZone
#  Address is reagan-VirtualBox:4078 (this may change with each run)

  writeXMsg(*StreamID,*Header,*Message);

  # now read the message      
  readXMsg(*StreamID,*Condition,*MessageNum,*SequenceNum,*MsgHeader,*MsgMessage,*Sender,*Address);  
  writeLine("stdout","Sequence Number is *SequenceNum");
  writeLine("stdout","Message Header is *MsgHeader");
  writeLine("stdout","Message is *MsgMessage");
  writeLine("stdout","Sender is *Sender");
  writeLine("stdout","Address is *Address");
}
INPUT *StreamID="1",*Header="Test",*Message="Body of the message",*Condition=``*XUSER == "rods@tempZone"``,*MessageNum="1" 
OUTPUT ruleExecOut
