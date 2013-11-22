myTestRule {
# Input parameters are: 
#   File descriptor 
#   Optional length to read  
# Output Parameter is: 
#   Buffer holding the data read 
# Output from running the example is: 
#  Open file /tempZone/home/rods/test/foo1, create file /tempZone/home/rods/test/foo4, copy 100 bytes starting at location 10    
   msiDataObjOpen(*OFlags,*S_FD);
   msiDataObjRead(*S_FD,*Len,*R_BUF);
   msiDataObjClose(*S_FD,*Status2);
} 
INPUT *Obj="/raja8/home/rods/msso/mssop1/mssop1.run", *Flag="O_RDONLY", *OFlags="objPath=*Obj++++openFlags=*Flag", *Len=100
OUTPUT ruleExecOut, *R_BUF
