myTestRule {
# Input parameters are: 
#   File descriptor 
#   Optional length to read  
# Output Parameter is: 
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
   writeLine("stdout","Open file *Obj, create file *ObjB, copy *Len bytes starting at location *Offset to *ObjB");
} 
INPUT *Nu="", *Obj="/tempZone/home/rods/test/foo1", *Resc="demoResc", *Repl="0", *Flag="O_RDONLY", *OFlags="objPath=*Obj++++rescName=*Resc++++replNum=*Repl++++openFlags=*Flag", *ObjB="/tempZone/home/rods/test/foo4", *OFlagsB="destRescName=*Resc++++forceFlag=*Nu", *Offset=10, *Loc="SEEK_SET", *Len=100
OUTPUT ruleExecOut
