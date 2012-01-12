myTestRule {
#Input parameter is:
#  Buffer to free (can be variable buffer or stdout or stderr)
  msiDataObjOpen(*Flags,*F_desc);
  msiDataObjRead(*F_desc,*Len,*Buf);
  msiDataObjClose(*F_desc,*Status);
  msiFreeBuffer(*Buf);
  writeLine("stdout","Freed buffer");
}
INPUT *Flags="objPath=/tempZone/home/rods/sub1/foo1", *Len="100"
OUTPUT ruleExecOut
