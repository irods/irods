myTestRule {
# Input parameters are:
#   File descriptor
#   Optional length to read
# Output parameter is:
#   Buffer holding the data read 
# Output from running the example is:
#  Open file /tempZone/home/rods/test/foo1, create file /tempZone/home/rods/test/foo4, copy 100 bytes starting at location 10
   msiDataObjOpen(*OFlags,*S_FD);
   msiDataObjCreate(*ObjB,*OFlagsB,*D_FD);
   msiDataObjLseek(*S_FD,*Offset,*Loc,*Status1);
   msiDataObjRead(*S_FD,*Len,*R_BUF);
   msiDataObjWrite(*D_FD,*R_BUF,*W_LEN);
   msiDataObjClose(*S_FD,*Status2);
   msiDataObjClose(*D_FD,*Status3);
   writeLine("stdout","Open file *Obj, create file *ObjB, copy *Len bytes starting at location *Offset");
} 
INPUT *Obj="/tempZone/home/rods/test/foo1", *OFlags="objPath=/tempZone/home/rods/test/foo1++++rescName=demoResc++++replNum=0++++openFlags=O_RDONLY", *ObjB="/tempZone/home/rods/test/foo4", *OFlagsB="destRescName=demoResc++++forceFlag=", *Offset="10", *Loc="SEEK_SET", *Len="100"
OUTPUT ruleExecOut
