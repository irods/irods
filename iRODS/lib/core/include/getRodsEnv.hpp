/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef GET_RODS_ENV
#define GET_RODS_ENV

extern "C" {

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
        char rodsClientServerPolicy[ LONG_NAME_LEN ];
        char rodsClientServerNegotiation[ LONG_NAME_LEN ];

        // =-=-=-=-=-=-=-
        // client side options for encryption
        int  rodsEncryptionKeySize;
        int  rodsEncryptionSaltSize;
        int  rodsEncryptionNumHashRounds;
        char rodsEncryptionAlgorithm[ HEADER_TYPE_LEN ];

    } rodsEnv;

    int getRodsEnv( rodsEnv *myRodsEnv );

    char *getRodsEnvFileName();
    char *getRodsEnvAuthFileName();

    int appendRodsEnv( char *appendText );

}

#endif	/* GET_RODS_ENV */
