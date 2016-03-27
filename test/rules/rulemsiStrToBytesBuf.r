myTestRule {
#Input parameters are:
#  String
#Output parameter is:
#  Buffer

  # Convert string to bytes buffer
  msiStrToBytesBuf(*Str,*Buf);

  # Create a file and write buffer into the file
  msiDataObjCreate(*Path,*Flags,*F_desc);
  msiDataObjWrite(*F_desc,*Buf,*Len);
  msiDataObjClose(*F_desc,*Status);

  # Write the string to stdout
  writeLine("stdout","Wrote *Str into file *Path");
  
  # Now read from the file
  msiDataObjOpen(*OFlags,*R_FD);
  msiDataObjRead(*R_FD,*Len+5,*R_BUF);
  msiDataObjClose(*R_FD,*junk);
  
  # Check that we get back what we entered
  if (str(*R_BUF) != *Str) {
  	fail;
  }
}
INPUT *Str="Test string for writing into a file", *Path="/tempZone/home/rods/sub1/foo2", *Flags="forceFlag=", *OFlags="objPath=*Path++++openFlags=O_RDONLY"
OUTPUT ruleExecOut
