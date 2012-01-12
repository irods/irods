myTestRule {
#Input parameter is:
#  Tag buffer
#Output parameter is:
#  Tag structure

  # Read in first 10,000 bytes of file
  msiDataObjOpen(*Pathfile,*F_desc);
  msiDataObjRead(*F_desc,*Len,*File_buf);
  msiDataObjClose(*F_desc,*Status);

  # Read in tag template
  msiDataObjOpen(*Tag,*T_desc);
  msiDataObjRead(*T_desc, 10000, *Tag_buf);
  msiReadMDTemplateIntoTagStruct(*Tag_buf,*Tags);
  msiDataObjClose(*T_desc,*Status);

  # Extract metadata from file using tag template
  msiExtractTemplateMDFromBuf(*File_buf,*Tags,*Keyval);

  # Write result to stdout
  writeKeyValPairs("stdout",*Keyval," : ");

  # Add metadata to the file
  msiGetObjType(*Outfile,*Otype);
  msiAssociateKeyValuePairsToObj(*Keyval,*Outfile,*Otype);
}  
INPUT *Tag="/tempZone/home/rods/test/email.tag", *Pathfile="/tempZone/home/rods/test/sample.email", *Outfile="/tempZone/home/rods/test/sample.email", *Len=10000 
OUTPUT ruleExecOut
