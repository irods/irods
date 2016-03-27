integrityACL {
#Rule to analyze files in a collection
#  Verify that a specific ACL is present on each file in collection
#Input
#  Collection that will be analyzed
#  Name of person to check for presence of ACL on file
#  Required ACL value, expressed as an integer
#Output
#  Names of files that are missing the required ACL

  #Verify input path is a collection
  *Result = 1;
  msiIsColl(*Coll,*Result,*Status);
  if(*Result == 0) {
    writeLine("stdout","Input path *Coll is not a collection");
    fail;
  }  

  #Get USER_ID for the input user name
  msiExecStrCondQuery("SELECT USER_ID where USER_NAME = '*User'",*GenQOut0);
  *Userid = "";
  foreach(*GenQOut0) {
    msiGetValByKey(*GenQOut0,"USER_ID", *Userid);
  }
  if(*Userid == "") {
    writeLine("stdout","Input user name *User is unknown");
    fail;
  }

  #Get DATA_ACCESS_DATA_ID number that corresponds to requested access control
  msiExecStrCondQuery("SELECT TOKEN_ID where TOKEN_NAMESPACE = 'access_type' and TOKEN_NAME = '*Acl'",*GenQOut1);
  foreach(*GenQOut1) {
    msiGetValByKey(*GenQOut1,"TOKEN_ID",*Access);
  }
  writeLine("stdout","Access control number of *Acl is *Access");
  *ContInxOld = 1;
  *Count = 0;

  #Loop over files in the collection
  msiMakeGenQuery("DATA_ID, DATA_NAME","COLL_NAME = '*Coll'", *GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    foreach(*GenQOut) {
      msiGetValByKey(*GenQOut, "DATA_ID", *Dataid);
      msiGetValByKey(*GenQOut, "DATA_NAME", *File);

      #Find ACL for each file
      msiMakeGenQuery("DATA_ACCESS_TYPE,DATA_ACCESS_USER_ID","DATA_ACCESS_DATA_ID = '*Dataid'", *GenQm);
      msiExecGenQuery(*GenQm, *GenQOutm);

      #Loop over metadata for each file
      *Attrfound = 0;
      foreach(*GenQOutm) {
        msiGetValByKey(*GenQOutm, "DATA_ACCESS_USER_ID", *Userdid);
        if(*Userdid == *Userid) {
          *Attrfound = 1;
          msiGetValByKey(*GenQOutm, "DATA_ACCESS_TYPE", *Datatype);
          if(*Datatype < *Access) {
            writeLine("stdout","*File has wrong access permission, *Datatype");
          }
        }
      } 
      if(*Attrfound == 0) {
        writeLine("stdout","*File is missing access controls for  *User");
        *Count = *Count + 1;
      }
    }
    *ContInxOld = *ContInxNew;
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }
  writeLine("stdout","Number of files in *Coll missing access control for *User is *Count");
}
INPUT *Coll = "/tempZone/home/rods", *User="rods", *Acl = "own"
OUTPUT ruleExecOut
