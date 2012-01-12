/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* l3bundle.c - test the low level api */

#include "rodsClient.h" 


#define BUFSIZE	(1024*1024)

int
main(int argc, char **argv)
{
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    int status;
    rErrMsg_t errMsg;
    subFile_t subFile;
    specColl_t specColl;
    subStructFileFdOprInp_t subStructFileFdOprInp;
    bytesBuf_t subStructFileReadOutBBuf;
    rodsStat_t *rodsStat = NULL;
    subStructFileLseekInp_t subStructFileLseekInp;
    fileLseekOut_t *subStructFileLseekOut = NULL;
    subStructFileRenameInp_t subStructFileRenameInp;
    rodsDirent_t *rodsDirent = NULL;
    structFileOprInp_t structFileOprInp;

    status = getRodsEnv (&myRodsEnv);

    if (status < 0) {
	fprintf (stderr, "getRodsEnv error, status = %d\n", status);
	exit (1);
    }

    conn = rcConnect (myRodsEnv.rodsHost, myRodsEnv.rodsPort, 
      myRodsEnv.rodsUserName, myRodsEnv.rodsZone, 0, &errMsg);

    if (conn == NULL) {
        fprintf (stderr, "rcConnect error\n");
        exit (1);
    }

    status = clientLogin(conn);
    if (status != 0) {
        fprintf (stderr, "clientLogin error\n");
       rcDisconnect(conn);
       exit (7);
    }

    memset (&structFileOprInp, 0, sizeof (structFileOprInp));
    structFileOprInp.specColl = malloc (sizeof (specColl_t));
    rstrcpy (structFileOprInp.specColl->collection, 
      "/tempZone/home/rods/dirx", MAX_NAME_LEN);
    structFileOprInp.specColl->collClass = STRUCT_FILE_COLL;
    structFileOprInp.specColl->type = TAR_STRUCT_FILE_T;
    rstrcpy (structFileOprInp.specColl->resource, "demoResc", NAME_LEN);
    rstrcpy (structFileOprInp.specColl->phyPath, 
      "/data/mwan/rods/Vault8/rods/tarfiles/my.tar", MAX_NAME_LEN);
    rstrcpy (structFileOprInp.specColl->cacheDir,
      "/data/mwan/rods/Vault8/rods/dirx", MAX_NAME_LEN);
    rstrcpy (structFileOprInp.addr.hostAddr, "srbbrick8.sdsc.edu", NAME_LEN);

    status = rcStructFileExtract (conn, &structFileOprInp);

    printf ("Porcessed rcStructFileExtract, status = %d\n", status);

    memset (&subFile, 0, sizeof (subFile));
    memset (&specColl, 0, sizeof (specColl));
    subFile.specColl = &specColl;
    rstrcpy (specColl.collection, "/tempZone/home/rods/dir1", MAX_NAME_LEN);
    specColl.collClass = STRUCT_FILE_COLL;
    specColl.type = HAAW_STRUCT_FILE_T;
    rstrcpy (specColl.objPath, "/tempZone/home/rods/dir1/myBundle", 
      MAX_NAME_LEN);
    rstrcpy (specColl.resource, "demoResc", NAME_LEN);
    rstrcpy (specColl.phyPath, "/data/mwan/rods/Vault8/rods/dir1/myBundle", 
      MAX_NAME_LEN);
    rstrcpy (subFile.subFilePath, "/tempZone/home/rods/dir1/mySubFile",
      MAX_NAME_LEN);
    rstrcpy (subFile.addr.hostAddr, "srbbrick8.sdsc.edu", NAME_LEN);

    status = rcSubStructFileOpen (conn, &subFile);

    printf ("Porcessed rcSubStructFileOpen, status = %d\n", status);

    status = rcSubStructFileCreate (conn, &subFile);
    
    printf ("Porcessed rcSubStructFileCreate, status = %d\n", status);

    memset (&subStructFileFdOprInp, 0, sizeof (subStructFileFdOprInp));

    rstrcpy (subStructFileFdOprInp.addr.hostAddr, "srbbrick8.sdsc.edu", NAME_LEN);
    subStructFileFdOprInp.type = HAAW_STRUCT_FILE_T;
    subStructFileFdOprInp.fd = 10;
    subStructFileFdOprInp.len = 1000;

    memset (&subStructFileReadOutBBuf, 0, sizeof (subStructFileReadOutBBuf));
    subStructFileReadOutBBuf.buf = malloc (subStructFileFdOprInp.len);

    status = rcSubStructFileRead (conn, &subStructFileFdOprInp, &subStructFileReadOutBBuf);

    printf ("Porcessed rcSubStructFileRead, status = %d\n", status);

    rstrcpy ((char *) subStructFileReadOutBBuf.buf, "This is a test from client", 
      subStructFileFdOprInp.len);

    subStructFileReadOutBBuf.len = subStructFileFdOprInp.len = 
      strlen ("This is a test from client") + 1;
    status = rcSubStructFileWrite (conn, &subStructFileFdOprInp, &subStructFileReadOutBBuf);

    printf ("Porcessed rcSubStructFileWrite, status = %d\n", status);

    status = rcSubStructFileClose (conn, &subStructFileFdOprInp);

    printf ("Porcessed rcSubStructFileClose, status = %d\n", status);

    status = rcSubStructFileStat (conn, &subFile, &rodsStat);

    printf ("Porcessed rcSubStructFileStat, status = %d\n", status);

    if (rodsStat != NULL) {
	free (rodsStat);
	rodsStat = NULL;
    }

    status = rcSubStructFileFstat (conn, &subStructFileFdOprInp, &rodsStat);

    printf ("Porcessed rcSubStructFileFstat, status = %d\n", status);
    
    if (rodsStat != NULL) {
        free (rodsStat);
        rodsStat = NULL;
    }

    memset (&subStructFileLseekInp, 0, sizeof (subStructFileLseekInp));

    rstrcpy (subStructFileLseekInp.addr.hostAddr, "srbbrick8.sdsc.edu", NAME_LEN);
    subStructFileLseekInp.type = HAAW_STRUCT_FILE_T;
    subStructFileLseekInp.fd = 10;
    subStructFileLseekInp.offset = 10000;
    subStructFileLseekInp.whence = 1;

    status = rcSubStructFileLseek (conn, &subStructFileLseekInp, &subStructFileLseekOut);

    printf ("Porcessed rcSubStructFileLseek, status = %d\n", status);

    if (subStructFileLseekOut != NULL) {
        free (subStructFileLseekOut);
        subStructFileLseekOut = NULL;
    }

    memset (&subStructFileRenameInp, 0, sizeof (subStructFileRenameInp));
    subStructFileRenameInp.subFile.specColl = &specColl;
    rstrcpy (subStructFileRenameInp.subFile.subFilePath, 
      "/tempZone/home/rods/dir1/mySubFile", MAX_NAME_LEN);
    rstrcpy (subStructFileRenameInp.subFile.addr.hostAddr, "srbbrick8.sdsc.edu", 
      NAME_LEN);
    rstrcpy (subStructFileRenameInp.newSubFilePath, 
      "/tempZone/home/rods/dir1/mySubFile2", MAX_NAME_LEN);

    status = rcSubStructFileRename (conn, &subStructFileRenameInp);

    printf ("Porcessed rcSubStructFileRename, status = %d\n", status);
    
    status = rcSubStructFileUnlink (conn, &subFile);
  
    printf ("Porcessed rcSubStructFileUnlink, status = %d\n", status);

    subFile.offset = 1000;
    status = rcSubStructFileTruncate (conn, &subFile);

    printf ("Porcessed rcSubStructFileTruncate, status = %d\n", status);

    status = rcSubStructFileMkdir (conn, &subFile);

    printf ("Porcessed rcSubStructFileMkdir, status = %d\n", status);

    status = rcSubStructFileRmdir (conn, &subFile);
    
    printf ("Porcessed rcSubStructFileRmdir, status = %d\n", status);

    status = rcSubStructFileOpendir (conn, &subFile);
   
    printf ("Porcessed rcSubStructFileOpendir, status = %d\n", status);

    status = rcSubStructFileReaddir (conn, &subStructFileFdOprInp, &rodsDirent);
  
    printf ("Porcessed rcSubStructFileReaddir, status = %d\n", status);

    status = rcSubStructFileClosedir (conn, &subStructFileFdOprInp);

    printf ("Porcessed rcSubStructFileClosedir, status = %d\n", status);

    rcDisconnect (conn);

    exit (0);
} 

