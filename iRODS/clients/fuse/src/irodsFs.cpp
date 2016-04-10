/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "iFuse.Lib.hpp"
#include "iFuse.Lib.Conn.hpp"
#include "iFuseOper.hpp"
#include "iFuse.Lib.RodsClientAPI.hpp"
#include "iFuseCmdLineOpt.hpp"

static struct fuse_operations irodsOper;

static void usage();
static int checkMountPoint(char *mountPoint, bool nonempty);

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
        return 1;
    }

    if (myRodsArgs.help == True) {
        usage();
        return 0;
    }
    if (myRodsArgs.version == True) {
        printf("Version: %s", RODS_REL_VERSION);
        return 0;
    }

    status = getRodsEnv(&myRodsEnv);
    if (status < 0) {
        iFuseRodsClientLogError(LOG_ERROR, status, "main: getRodsEnv error.");
        return 1;
    }
    
    iFuseCmdOptsInit();

    iFuseCmdOptsParse(argc, argv);
    iFuseCmdOptsAdd("-odirect_io");

    iFuseGetOption(&myiFuseOpt);
    
    // check mount point
    status = checkMountPoint(myiFuseOpt.mountpoint, myiFuseOpt.nonempty);
    if(status != 0) {
        fprintf(stderr, "iRods Fuse abort\n");
        iFuseCmdOptsDestroy();
        return 1;
    }

    iFuseLibSetRodsEnv(&myRodsEnv);
    iFuseLibSetOption(&myiFuseOpt);

    // check iRODS iCAT host connectivity
    status = iFuseConnTest();
    if(status != 0) {
        fprintf(stderr, "iRods Fuse abort: cannot connect to iCAT\n");
        iFuseCmdOptsDestroy();
        return 1;
    }
    
    iFuseGenCmdLineForFuse(&fuse_argc, &fuse_argv);
    
    iFuseRodsClientLog(LOG_DEBUG, "main: iRods Fuse gets started.");
    status = fuse_main(fuse_argc, fuse_argv, &irodsOper, NULL);
    iFuseReleaseCmdLineForFuse(fuse_argc, fuse_argv);

    iFuseCmdOptsDestroy();

    if (status < 0) {
        return 1;
    } else {
        return 0;
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

static int checkMountPoint(char *mountPoint, bool nonempty) {
    DIR *dir = NULL;
    char cwd[MAX_NAME_LEN];
    char mpath[MAX_NAME_LEN];
    char mpathabs[MAX_NAME_LEN];
    
    if(getcwd(cwd, sizeof(cwd)) != NULL) {
        if(mountPoint[0] == '/') {
            strcpy(mpath, mountPoint);
        } else {
            strcpy(mpath, cwd);
            if(mpath[strlen(mpath)-1] != '/') {
                strcat(mpath, "/");
            }
            strcat(mpath, mountPoint);
        }
    } else {
        fprintf(stderr, "Cannot get a current directory\n");
        return -1;
    }
    
    realpath(mpath, mpathabs);
    if(mpathabs[strlen(mpathabs)-1] != '/') {
        strcat(mpathabs, "/");
    }
    
    dir = opendir(mpathabs);
    if(dir != NULL) {
        // exists
        bool filefound = false;
        if(!nonempty) {
            // check if directory is empty or not
            struct dirent *d;
            while ((d = readdir(dir)) != NULL) {
                if(!strcmp(d->d_name, ".")) {
                    continue;
                } else if(!strcmp(d->d_name, "..")) {
                    continue;
                } else {
                    filefound = true;
                    break;
                }
            }
        }
        
        closedir(dir);
        
        if(filefound) {
            fprintf(stderr, "A directory %s is not empty\nif you are sure this is safe, use the 'nonempty' mount option", mpathabs);
            return -1;
        }
    } else if(errno == ENOENT) {
        // directory not accessible
        fprintf(stderr, "Cannot find a directory %s\n", mpathabs);
        return -1;
    } else {
        // not accessible
        fprintf(stderr, "The directory %s is not accessible\n", mpathabs);
        return -1;
    }
    
    return 0;
}