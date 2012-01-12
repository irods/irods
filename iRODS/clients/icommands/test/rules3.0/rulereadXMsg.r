myTestRule {
#Input parameters are:
#  Stream ID 
#  Condition for reading a packet
#    *XADDR != "srbbrick14:20135"   - form host:process-id
#    *XUSER != "rods@tempZone"      - form user@zone
#    *XHDR == "header"              - header of the message
#    *XMISC == "msic_infor"         - miscellaneous message part
#    *XTIME                       - Unix time in seconds from Jan 1, 1968
#Output parameters are:
#  Message number
#  Sequence number from the Xmsg Server
#  Header string
#  Message
#  Sender
#  Sending site
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
