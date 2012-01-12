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
}
INPUT *Str="Test string for writing into a file", *Path="/tempZone/home/rods/sub1/foo2", *Flags="forceFlag="
OUTPUT ruleExecOut
