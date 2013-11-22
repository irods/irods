myTestRule {
   msiExecCmd("touch", *File, "null","null","null",*Result);
   writeLine("serverLog", "second test touched file *File");
   writeLine("stdout", "second test touched file *File");
   msiDataObjOpen("objPath=/raja8/home/rods/msso/mssop1/mssop1.run++++openFlags=O_RDONLY",*S_FD);
   msiDataObjRead(*S_FD,*Len,*R_BUF);
   msiDataObjClose(*S_FD,*Status2);
} 
INPUT 
