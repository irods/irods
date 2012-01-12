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
generateBagIt {
### - creates NEWBAGITROOT
  msiCollCreate(*NEWBAGITROOT, 1, *junk);
### - writes bagit.txt to NEWBAGITROOT/bagit.txt
  writeLine(stdout, "BagIt-Version: 0.96");
  writeLine(stdout, "Tag-File-Character-Encoding: UTF-8");
  msiDataObjCreate(*NEWBAGITROOT/bagit.txt, null, *FD);
  msiDataObjWrite(*FD, stdout, *WLEN);
  msiDataObjClose(*FD, *junk);
  msiFreeBuffer(stdout);
### - rsyncs existing BAGITDATA to NEWBAGITROOT/data
  msiCollRsync(*BAGITDATA, *NEWBAGITROOT/data, null, IRODS_TO_IRODS, *junk);
### - generates payload manifest file of NEWBAGITROOT/data
  assign(*NEWBAGITDATA,*NEWBAGITROOT/data);
  msiCollectionSpider(*NEWBAGITDATA, *Object, "msiDataObjChksum(*Object, forceChksum, *CHKSUM)##msiGetObjectPath(*Object,*FULLPATH,*junk)##msiStrlen(*NEWBAGITROOT,*ROOTLENGTH)##assign(*OFFSET,\"*ROOTLENGTH + 1\")##msiSubstr(*FULLPATH,*OFFSET,null,*RELATIVEPATH)##writeString(stdout, *RELATIVEPATH)##writeLine(stdout,\"   *CHKSUM\")", *Status);
### - writes payload manifest to NEWBAGITROOT/manifest-md5.txt
  msiDataObjCreate(*NEWBAGITROOT/manifest-md5.txt, null, *FD);
  msiDataObjWrite(*FD, stdout, *WLEN);
  msiDataObjClose(*FD, *junk);
  msiFreeBuffer(stdout);
### - writes tagmanifest file to NEWBAGITROOT/tagmanifest-md5.txt
  writeString(stdout, "bagit.txt    ");
  msiDataObjChksum(*NEWBAGITROOT/bagit.txt, forceChksum, *CHKSUM);
  writeLine(stdout, *CHKSUM);
  writeString(stdout, "manifest-md5.txt    ");
  msiDataObjChksum(*NEWBAGITROOT/manifest-md5.txt, forceChksum, *CHKSUM);
  writeLine(stdout, *CHKSUM);
  msiDataObjCreate(*NEWBAGITROOT/tagmanifest-md5.txt, null, *FD);
  msiDataObjWrite(*FD, stdout, *WLEN);
  msiDataObjClose(*FD, *junk);
  msiFreeBuffer(stdout);
### - creates tarfile of new bag for faster download
  msiTarFileCreate(*NEWBAGITROOT.tar, *NEWBAGITROOT, null, *junk);
### - gets filesize of new tarfile
  msiSplitPath(*NEWBAGITROOT.tar,*PATH,*TARFILENAME);
  msiMakeQuery("DATA_SIZE", "COLL_NAME like '*PATH%%' AND DATA_NAME = '*TARFILENAME'", *Query);
  msiExecStrCondQuery(*Query, *E);
  forEach(*E) {
    msiGetValByKey(*E,DATA_SIZE,*FILESIZE);
  }
  if (*FILESIZE > 1048576) then {
    assign(*PRINTSIZE, "*FILESIZE / 1048576");
    assign(*PRINTUNIT, "MB");
  }
  else {
    if (*FILESIZE > 1024) then {
      assign(*PRINTSIZE, "*FILESIZE / 1024");
      assign(*PRINTUNIT, "KB");
    }
    else {
      assign(*PRINTSIZE, *FILESIZE);
      assign(*PRINTUNIT, "B");
    }
  }
### - outputs report and suggested download procedures
  writeLine(stdout, "");
  writeLine(stdout, "Your BagIt bag has been created and tarred on the iRODS server:");
  writeLine(stdout, "  *NEWBAGITROOT.tar - *PRINTSIZE *PRINTUNIT");
  writeLine(stdout, "");
  msiSplitPath(*NEWBAGITROOT.tar,*COLL,*TARFILE);
  writeLine(stdout, "To copy it to your local computer, use:");
  writeLine(stdout, "  iget -Pf *NEWBAGITROOT.tar *TARFILE");
  writeLine(stdout, "");
### - writes to rodsLog
  msiWriteRodsLog("BagIt bag created: *NEWBAGITROOT <- *BAGITDATA", *junk);
}
INPUT *BAGITDATA="$", *NEWBAGITROOT="$"
OUTPUT ruleExecOut

