/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef GET_RODS_ENV
#define GET_RODS_ENV

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
   char rodsUserName[NAME_LEN];
   char rodsHost[NAME_LEN];
   int  rodsPort;
   char xmsgHost[NAME_LEN];
   int  xmsgPort;
   char rodsHome[MAX_NAME_LEN];
   char rodsCwd[MAX_NAME_LEN];
   char rodsAuthScheme[NAME_LEN];
   char rodsDefResource[NAME_LEN];
   char rodsZone[NAME_LEN];
   char *rodsServerDn;
   int rodsLogLevel;
   char rodsAuthFileName[LONG_NAME_LEN];
   char rodsDebug[NAME_LEN];
} rodsEnv;

int getRodsEnv(rodsEnv *myRodsEnv);

char *getRodsEnvFileName();
char *getRodsEnvAuthFileName();

int appendRodsEnv(char *appendText);

#ifdef  __cplusplus
}
#endif

#endif	/* GET_RODS_ENV */
