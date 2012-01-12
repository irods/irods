generateBagIt {
# -----------------------------------------------------
# generateBagIt
# -----------------------------------------------------
#
#  Terrell Russell
#  University of North Carolina at Chapel Hill
#  - August 2010
#  - Requires iRODS 2.4.1
#  - Conforms to BagIt Spec v0.96
#
# -----------------------------------------------------
#
### - creates NEWBAGITROOT
### - writes bagit.txt to NEWBAGITROOT/bagit.txt
### - rsyncs existing BAGITDATA to NEWBAGITROOT/data
### - generates payload manifest file of NEWBAGITROOT/data
### - writes payload manifest to NEWBAGITROOT/manifest-md5.txt
### - writes tagmanifest file to NEWBAGITROOT/tagmanifest-md5.txt
### - creates tarfile of new bag for faster download
### - gets filesize of new tarfile
### - outputs report and suggested download procedures
### - writes to rodsLog
#
# -----------------------------------------------------

  ### - creates NEWBAGITROOT
  msiCollCreate(*NEWBAGITROOT,"1",*Status);
  msiStrlen(*NEWBAGITROOT,*ROOTLENGTH);
  *OFFSET = int(*ROOTLENGTH) + 1;

  ### - writes bagit.txt to NEWBAGITROOT/bagit.txt
  writeLine("stdout","BagIt-Version: 0.96");
  writeLine("stdout","Tag-File-Character-Encoding: UTF-8");
  msiDataObjCreate("*NEWBAGITROOT" ++ "/bagit.txt","null",*FD);
  msiDataObjWrite(*FD,"stdout",*WLEN);
  msiDataObjClose(*FD,*Status);
  msiFreeBuffer("stdout");

  ### - rsyncs existing BAGITDATA to NEWBAGITROOT/data
  msiCollRsync(*BAGITDATA,"*NEWBAGITROOT" ++ "/data","null","IRODS_TO_IRODS",*Status);

  ### - generates payload manifest file of NEWBAGITROOT/data
  *NEWBAGITDATA = "*NEWBAGITROOT" ++ "/data";
  *ContInxOld = 1;
  *Condition = "COLL_NAME like '*NEWBAGITDATA%%'";
  msiMakeGenQuery("DATA_ID, DATA_NAME, COLL_NAME",*Condition,*GenQInp);
  msiExecGenQuery(*GenQInp, *GenQOut);
  msiGetContInxFromGenQueryOut(*GenQOut,*ContInxNew);
  while(*ContInxOld > 0) {
    foreach(*GenQOut) {
       msiGetValByKey(*GenQOut, "DATA_NAME", *Object);
       msiGetValByKey(*GenQOut, "COLL_NAME", *Coll);
       *FULLPATH = "*Coll" ++ "/" ++ "*Object";
       msiDataObjChksum(*FULLPATH, "forceChksum=", *CHKSUM);
       msiSubstr(*FULLPATH,str(*OFFSET),"null",*RELATIVEPATH);
       writeString("stdout", *RELATIVEPATH);
       writeLine("stdout","   *CHKSUM")
    }
    *ContInxOld = *ContInxNew;
    if(*ContInxOld > 0) {msiGetMoreRows(*GenQInp,*GenQOut,*ContInxNew);}
  }

  ### - writes payload manifest to NEWBAGITROOT/manifest-md5.txt
  msiDataObjCreate("*NEWBAGITROOT" ++ "/manifest-md5.txt","null",*FD);
  msiDataObjWrite(*FD,"stdout",*WLEN);
  msiDataObjClose(*FD,*Status);
  msiFreeBuffer("stdout");

  ### - writes tagmanifest file to NEWBAGITROOT/tagmanifest-md5.txt
  writeString("stdout","bagit.txt    ");
  msiDataObjChksum("*NEWBAGITROOT" ++ "/bagit.txt","forceChksum",*CHKSUM);
  writeLine("stdout",*CHKSUM);
  writeString("stdout","manifest-md5.txt    ");
  msiDataObjChksum("*NEWBAGITROOT" ++ "/manifest-md5.txt","forceChksum",*CHKSUM);
  writeLine("stdout",*CHKSUM);
  msiDataObjCreate("*NEWBAGITROOT" ++ "/tagmanifest-md5.txt","null",*FD);
  msiDataObjWrite(*FD,"stdout",*WLEN);
  msiDataObjClose(*FD,*Status);
  msiFreeBuffer("stdout");

  ### - creates tarfile of new bag for faster download
  msiTarFileCreate("*NEWBAGITROOT" ++ ".tar",*NEWBAGITROOT,"null",*Status);

  ### - gets filesize of new tarfile
  msiSplitPath("*NEWBAGITROOT" ++ ".tar",*Coll,*TARFILENAME);
  msiMakeQuery("DATA_SIZE","COLL_NAME like '*Coll%%' AND DATA_NAME = '*TARFILENAME'",*Query);
  msiExecStrCondQuery(*Query,*E);
  foreach(*E) {
    msiGetValByKey(*E,"DATA_SIZE",*FILESIZE);
    *Isize = int(*FILESIZE);
    if(*Isize > 1048576) {
      *PRINTSIZE = *Isize / 1048576;
      *PRINTUNIT = "MB";
    }
    else {
      if(*Isize > 1024) {
        *PRINTSIZE = *Isize / 1024;
        *PRINTUNIT = "KB";
      }
      else {
        *PRINTSIZE = *Isize;
        *PRINTUNIT = "B";
      }
    }
  }

  ### - outputs report and suggested download procedures
  writeLine("stdout","");
  writeLine("stdout","Your BagIt bag has been created and tarred on the iRODS server:")
  writeLine("stdout","  *NEWBAGITROOT.tar - *PRINTSIZE *PRINTUNIT");
  writeLine("stdout","");
  msiSplitPath("*NEWBAGITROOT" ++ ".tar",*COLL,*TARFILE);
  writeLine("stdout","To copy it to your local computer, use:");
  writeLine("stdout","  iget -Pf *NEWBAGITROOT.tar *TARFILE");
  writeLine("stdout","");

  ### - writes to rodsLog
  msiWriteRodsLog("BagIt bag created: *NEWBAGITROOT <- *BAGITDATA",*Status);
}
INPUT *BAGITDATA=$"/tempZone/home/rods/sub1", *NEWBAGITROOT=$"/tempZone/home/rods/bagit"
OUTPUT ruleExecOut
