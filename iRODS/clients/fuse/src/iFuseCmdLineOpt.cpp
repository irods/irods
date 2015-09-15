/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is written by Illyoung Choi (iychoi@email.arizona.edu)      ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "iFuseCmdLineOpt.hpp"
#include "iFuse.Lib.Conn.hpp"
#include "miscUtil.h"

static iFuseOpt_t g_Opt;

void iFuseCmdOptsInit() {
    bzero(&g_Opt, sizeof(iFuseOpt_t));

    g_Opt.bufferedFS = true;
    g_Opt.preload = true;
    g_Opt.maxConn = IFUSE_MAX_NUM_CONN;
    g_Opt.connTimeoutSec = IFUSE_FREE_CONN_TIMEOUT_SEC;
    g_Opt.connKeepAliveSec = IFUSE_FREE_CONN_KEEPALIVE_SEC;
}

void iFuseCmdOptsDestroy() {
    iFuseExtendedOpt_t *peopt = NULL;

    if(g_Opt.program != NULL) {
        free(g_Opt.program);
        g_Opt.program = NULL;
    }

    if(g_Opt.mountpoint != NULL) {
        free(g_Opt.mountpoint);
        g_Opt.mountpoint = NULL;
    }

    peopt = g_Opt.extendedOpts;
    while(peopt != NULL) {
        iFuseExtendedOpt_t *next = peopt->next;

        if(peopt->opt != NULL) {
            free(peopt->opt);
            peopt->opt = NULL;
        }
        free(peopt);

        peopt = next;
    }
    g_Opt.extendedOpts = NULL;
}

iFuseExtendedOpt_t *_newExtendedOpt(char *opt) {
    iFuseExtendedOpt_t *eopt = NULL;
    eopt = (iFuseExtendedOpt_t *)calloc(1, sizeof(iFuseExtendedOpt_t));
    if(eopt != NULL) {
        eopt->opt = strdup(opt);
    }
    return eopt;
}

void iFuseCmdOptsParse(int argc, char **argv) {
    int c;
    char buff[MAX_NAME_LEN];
    int index;
    int i;

    g_Opt.program = strdup(argv[0]);

    for(i=0;i<argc;i++) {
        if(strcmp("-onocache", argv[i]) == 0) {
            g_Opt.bufferedFS = false;
            g_Opt.preload = false;
            argv[i] = "-Z";
        } else if(strcmp("-onopreload", argv[i]) == 0) {
            g_Opt.preload = false;
            argv[i] = "-Z";
        } else if(strcmp("-omaxconn", argv[i]) == 0) {
            if(argc > i+1) {
                g_Opt.maxConn = atoi(argv[i+1]);
                argv[i+1] = "-Z";
            }
            argv[i] = "-Z";
        } else if(strcmp("-oconntimeout", argv[i]) == 0) {
            if(argc > i+1) {
                g_Opt.connTimeoutSec = atoi(argv[i+1]);
                argv[i+1] = "-Z";
            }
            argv[i] = "-Z";
        } else if(strcmp("-oconnkeepalive", argv[i]) == 0) {
            if(argc > i+1) {
                g_Opt.connKeepAliveSec = atoi(argv[i+1]);
                argv[i+1] = "-Z";
            }
            argv[i] = "-Z";
        }
    }

    optind = 1;
    while((c = getopt(argc, argv, "dfo:Z")) != -1) {
        switch(c) {
            case 'd':
                {
                    // fuse debug mode
                    g_Opt.debug = true;
                }
                break;
            case 'f':
                {
                    // fuse foreground mode
                    g_Opt.foreground = true;
                }
                break;
            case 'o':
                {
                    // fuse options
                    if (!strcmp("use_ino", optarg)) {
                        fprintf(stderr, "use_ino fuse option not supported, ignoring\n");
                        break;
                    }
                    if (!strcmp("nonempty", optarg)) {
                        // fuse nonempty option
                        g_Opt.nonempty = true;
                        break;
                    }
                    bzero(buff, MAX_NAME_LEN);
                    sprintf(buff, "-o%s", optarg);
                    iFuseCmdOptsAdd(buff);
                }
                break;
            case 'Z':
                // ignore (from irods parse)
                break;
            case '?':
                fprintf(stderr, "%c %s", c, optarg);
                break;
            default:
                fprintf(stderr, "%c %s", c, optarg);
                break;
        }
    }

    for(index=optind;index<argc;index++) {
        assert(g_Opt.mountpoint == NULL);

        g_Opt.mountpoint = strdup(argv[index]);
    }
}

void iFuseCmdOptsAdd(char *opt) {
    iFuseExtendedOpt_t *eopt = _newExtendedOpt(opt);
    assert(eopt != NULL);

    if(strncmp(opt, "-o", 2) == 0) {
        // starting with -o (fuse options)
        eopt->fuse = true;
    } else {
        eopt->fuse = false;
    }

    // add to list
    eopt->next = g_Opt.extendedOpts;
    g_Opt.extendedOpts = eopt;
}

void iFuseGetOption(iFuseOpt_t *opt) {
    assert(opt != NULL);

    memcpy(opt, &g_Opt, sizeof(iFuseOpt_t));
    opt->extendedOpts = NULL;
}

void iFuseGenCmdLineForFuse(int *fuse_argc, char ***fuse_argv) {
    int i;
    int argc = 0;
    char **argv = NULL;
    iFuseExtendedOpt_t *peopt = NULL;

    *fuse_argc = 0;
    *fuse_argv = NULL;

    if(g_Opt.program) {
        argc++;
    }

    if(g_Opt.debug) {
        argc++;
    }

    if(g_Opt.foreground) {
        argc++;
    }

    if(g_Opt.mountpoint != NULL) {
        argc++;
    }

    peopt = g_Opt.extendedOpts;
    while(peopt != NULL) {
        if(peopt->fuse) {
            if(peopt->opt != NULL) {
                argc++;
            }
        }
        peopt = peopt->next;
    }

    argv = (char**)calloc(argc, sizeof(char*));
    assert(argv != NULL);

    i = 0;
    if(g_Opt.program) {
        argv[i] = strdup(g_Opt.program);
        i++;

        if(g_Opt.debug) {
            fprintf(stdout, "prog : %s\n", g_Opt.program);
        }
    }

    if(g_Opt.debug) {
        argv[i] = strdup("-d");
        i++;

        fprintf(stdout, "debug : %d\n", g_Opt.debug);
    }

    if(g_Opt.foreground) {
        argv[i] = strdup("-f");
        i++;

        if(g_Opt.debug) {
            fprintf(stdout, "foreground : %d\n", g_Opt.foreground);
        }
    }
    
    if(g_Opt.nonempty) {
        argv[i] = strdup("-nonempty");
        i++;

        if(g_Opt.debug) {
            fprintf(stdout, "nonempty : %d\n", g_Opt.nonempty);
        }
    }

    peopt = g_Opt.extendedOpts;
    while(peopt != NULL) {
        if(peopt->fuse) {
            if(peopt->opt != NULL) {
                argv[i] = strdup(peopt->opt);
                i++;

                if(g_Opt.debug) {
                    fprintf(stdout, "opt : %s\n", peopt->opt);
                }
            }
        }
        peopt = peopt->next;
    }

    if(g_Opt.mountpoint != NULL) {
        argv[i] = strdup(g_Opt.mountpoint);
        i++;

        if(g_Opt.debug) {
            fprintf(stdout, "mountpoint : %s\n", g_Opt.mountpoint);
        }
    }

    *fuse_argc = argc;
    *fuse_argv = argv;
}

void iFuseReleaseCmdLineForFuse(int fuse_argc, char **fuse_argv) {
    int i;
    if (fuse_argv != NULL) {
        for (i=0;i<fuse_argc;i++) {
            if (fuse_argv[i] != NULL) {
                free(fuse_argv[i]);
            }
        }
        free(fuse_argv);
    }
}
