myTestRule {
   msiExecCmd("touch", *File, "null","null","null",*Result);
   writeLine("serverLog", "second test touched file *File");
   writeLine("stdout", "second test touched file *File");
   acRunWorkFlow("/raja8/home/rods/msso/mssop1/mssop1.run",*R_BUF);
} 
INPUT 
