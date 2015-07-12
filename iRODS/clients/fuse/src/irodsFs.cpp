/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "iFuse.Lib.hpp"
#include "iFuseOper.hpp"
#include "iFuseCmdLineOpt.hpp"

static struct fuse_operations irodsOper;

static void usage();

int main(int argc, char **argv) {
    int status;
    rodsArguments_t myRodsArgs;
    char *optStr;
    rodsEnv myRodsEnv;
    int fuse_argc;
    char **fuse_argv;
    iFuseOpt_t myiFuseOpt;

    bzero(&irodsOper, sizeof ( irodsOper));
    irodsOper.getattr = iFuseGetAttr;
    irodsOper.readlink = iFuseReadLink;
    irodsOper.mkdir = iFuseMakeDir;
    irodsOper.unlink = iFuseUnlink;
    irodsOper.rmdir = iFuseRemoveDir;
    irodsOper.open = iFuseOpen;
    irodsOper.read = iFuseRead;
    irodsOper.write = iFuseWrite;
    irodsOper.statfs = iFuseStatfs;
    irodsOper.release = iFuseClose;
    irodsOper.opendir = iFuseOpenDir;
    irodsOper.readdir = iFuseReadDir;
    irodsOper.releasedir = iFuseCloseDir;
    irodsOper.init = iFuseInit;
    irodsOper.destroy = iFuseDestroy;
    irodsOper.link = iFuseLink;
    irodsOper.symlink = iFuseSymlink;
    irodsOper.rename = iFuseRename;
    irodsOper.utimens = iFuseUtimens;
    irodsOper.truncate = iFuseTruncate;
    irodsOper.chmod = iFuseChmod;
    irodsOper.chown = iFuseChown;
    irodsOper.flush = iFuseFlush;
    irodsOper.mknod = iFuseCreate;
    irodsOper.fsync = iFuseFsync;
    
    optStr = "Zhdo:";

    status = parseCmdLineOpt(argc, argv, optStr, 1, &myRodsArgs);
    if (status < 0) {
        printf("Use -h for help.\n");
        exit(1);
    }

    if (myRodsArgs.help == True) {
        usage();
        exit(0);
    }
    if (myRodsArgs.version == True) {
        printf("Version: %s", RODS_REL_VERSION);
        exit(0);
    }

    status = getRodsEnv(&myRodsEnv);
    if (status < 0) {
        rodsLogError(LOG_ERROR, status, "main: getRodsEnv error. ");
        exit(1);
    }

    srandom((unsigned int) time(0) % getpid());

    iFuseCmdOptsInit();
    
    iFuseCmdOptsParse(argc, argv);
    iFuseCmdOptsAdd("-odirect_io");
#if defined macintosh || defined Macintosh || defined __APPLE__
    // always run in single threaded mode (this is because osxfuse has an issue of use of pthread)
    iFuseCmdOptsAdd("-os");
#endif
    
    iFuseGetOption(&myiFuseOpt);
    
    iFuseLibSetRodsEnv(&myRodsEnv);
    iFuseLibSetOption(&myiFuseOpt);
    
    iFuseGenCmdLineForFuse(&fuse_argc, &fuse_argv);
    status = fuse_main(fuse_argc, fuse_argv, &irodsOper, NULL);
    iFuseReleaseCmdLineForFuse(fuse_argc, fuse_argv);
    
    iFuseCmdOptsDestroy();
    
    if (status < 0) {
        exit(3);
    } else {
        exit(0);
    }
}

static void usage() {
    char *msgs[] = {
        "Usage: irodsFs [-hd] [-o opt,[opt...]]",
        "Single user iRODS/Fuse server",
        "Options are:",
        " -h        this help",
        " -d        FUSE debug mode",
        " -o        opt,[opt...]  FUSE mount options",
        " --version print version information",
        ""
    };
    int i;
    for (i = 0;; i++) {
        if (strlen(msgs[i]) == 0) {
            return;
        }
        printf("%s\n", msgs[i]);
    }
}
