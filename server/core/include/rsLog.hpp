/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsLog.h - header file for rsLog.c
 */

#ifndef RS_LOG_HPP
#define RS_LOG_HPP

#include "rods.h"

// clang-format off

#define RODS_LOGFILE        "rodsLog"
#define RODS_SERVER_LOGFILE "rodsServerLog"
#define RULE_EXEC_LOGFILE   "reLog"

#define DEF_LOG_DIR         "../../var/lib/irods/log"
#define PROC_LOG_DIR_NAME   "proc"

// =-=-=-=-=-=-=-
// JMC - backport 4793
#define DEF_LOGFILE_INT     5                   /* default interval in days */
#define LOGFILE_INT         "logfileInt"        /* interval in days for new log file */


#define LOGFILE_CHK_INT     1800                /* Interval in sec for checking logFile */
#define LOGFILE_CHK_CNT     50                  /* number of times through the loop before chkLogfileName is called */
#define DEF_LOGFILE_PATTERN "%Y.%m.%d"          /* default pattern in strftime syntax */
#define LOGFILE_PATTERN     "logfilePattern"    /* pattern for new name of log file */
// =-=-=-=-=-=-=-

// clang-format on

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char* getLogDir();

void getLogfileName( char **logFile, const char *logDir, const char *logFileName );

int chkLogfileName( const char *logDir, const char *logFileName );

#endif /* RS_LOG_H */
