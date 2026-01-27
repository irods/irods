myTestRule {
#Input parameters are:
#  Buffer
#  Tag structure
#Output parameter is:
#  Keyval pair buffer

  #Read in 10,000 bytes of the file
  msiDataObjOpen(*Pathfile,*F_desc);
  msiDataObjRead(*F_desc,*Len,*File_buf);
  msiDataObjClose(*F_desc,*Status);

  #Read in the tag template file
  msiDataObjOpen(*Tag,*T_desc);
  msiDataObjRead(*T_desc, 10000, *Tag_buf);
  msiReadMDTemplateIntoTagStruct(*Tag_buf,*Tags);
  msiDataObjClose(*T_desc,*Status);

  #Extract metadata from file using the tag template file
  msiExtractTemplateMDFromBuf(*File_buf,*Tags,*Keyval);

  #Write out extracted metadata
  writeKeyValPairs("stdout",*Keyval," : ");
  msiGetObjType(*Outfile,*Otype);

  #Add metadata to the object
  msiAssociateKeyValuePairsToObj(*Keyval,*Outfile,*Otype);
}  
INPUT *Tag="/tempZone/home/rods/test/email.tag", *Pathfile="/tempZone/home/rods/test/sample.email", *Outfile="/tempZone/home/rods/test/sample.email", *Len=10000 
OUTPUT ruleExecOut
