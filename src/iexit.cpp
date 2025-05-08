#include <irods/irods_exception.hpp>
#include <irods/irods_service_account.hpp>
#include <irods/parseCommandLine.h>
#include <irods/rcMisc.h>
#include <irods/rods.h>

#include <cstdio>

void usage( char *prog );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    rodsArguments_t myRodsArgs;

    int status = parseCmdLineOpt(argc, argv, "fVvh", 0, &myRodsArgs);
    if ( status ) {
        printf( "Use -h for help\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage( argv[0] );
        exit( 0 );
    }

    rodsEnv myEnv;
    getRodsEnv(&myEnv);
    const char* envFile = getRodsEnvFileName();
    bool isServiceAccountError = false;
    bool isServiceAccount = false;
    try {
        isServiceAccount = irods::is_service_account();
    }
    catch (const irods::exception &e) {
        isServiceAccountError = true;
        isServiceAccount = true; // to make if/else blocks neater
        std::fprintf(stderr,
                     "WARNING: Could not determine whether %s is running as service account "
                     "due to thrown exception: %s\n",
                     argv[0],
                     e.client_display_what());
    }

    if (myRodsArgs.verbose == True) {
        std::printf("Deleting (if it exists) session envFile: %s\n", envFile);
    }
    status = unlink(envFile);
    if (myRodsArgs.verbose == True) {
        std::printf("unlink status [%d]\n", status);
    }

    if (myRodsArgs.verbose == True && !isServiceAccount) {
        std::printf("Deleting (if it exists) session auth file\n");
    }
    if (myRodsArgs.force == True) {
        if (isServiceAccount) {
            if (isServiceAccountError) {
                std::printf("WARNING: Cannot determine if %s is running as service account, "
                            "but deleting auth file anyway.\n",
                            argv[0]);
            }
            else {
                std::printf("WARNING: %s appears to be running as service account, "
                            "but deleting auth file anyway.\n",
                            argv[0]);
            }
        }
        // do not prompt
        status = obfRmPw(1);
        if (myRodsArgs.verbose == True) {
            std::printf("unlink status [%d]\n", status);
        }
    }
    else if (isServiceAccount) {
        if (isServiceAccountError) {
            std::printf("WARNING: Cannot determine if %s is running as service account, "
                        "Skipping auth file deletion (pass -f to force).\n",
                        argv[0]);
        }
        else {
            std::printf("WARNING: %s appears to be running as service account. "
                        "Skipping auth file deletion (pass -f to force).\n",
                        argv[0]);
        }
    }
    else if (myRodsArgs.verbose == True) {
        // prompt
        status = obfRmPw(0);
        std::printf("unlink status [%d]\n", status);
    }
    else {
        // do not prompt
        obfRmPw( 1 );
    }

    exit( 0 );
}


void usage(char *prog) {
    std::printf("Exits iRODS session (cwd) and removes\n");
    std::printf("the scrambled password file produced by iinit.\n");
    std::printf("Usage: %s [-fvVh]\n", prog);
    std::printf(" -f  force removal of auth file\n");
    std::printf(" -v  verbose\n");
    std::printf(" -V  Very verbose\n");
    std::printf(" -h  this help\n");
    printReleaseInfo("iexit");
}
