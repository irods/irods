#ifndef _IRODSNTUTIL_H_
#define _IRODSNTUTIL_H_

#include <string.h>
/*
#include <io.h>
#include <fcntl.h>
#include <windows.h>
*/
#include "stdlib.h"
#include "stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <conio.h>
#include <direct.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define irodsntstat __stat64 /* _stat */

void iRODSNtAgentInit(int ac, char **av);
int iRODSNtGetServiceName(char *service_name);
void iRODSNtServerCheckExecMode(int targc, char **targv);
int iRODSNtServerRunningConsoleMode();
void iRODSNtSetServerHomeDir(char *inExec);
void iRODSNtGetAgentExecutableWithPath(char *buf, char *agent_name);
char *iRODSNtGetServerConfigPath();
void iRODSNtGetLogFilename(char *log_filename);
void iRODSNtGetLogFilenameWithPath(char *logfilename_wp);
char *iRODSNtServerGetLogDir();
void iRODSPathToNtPath(char *ntpath,const char *srbpath);
int iRODSNtFileOpen(const char *filename,int oflag, int istextfile);
void iRODSNtPathBackSlash(char *str);
void iRODSNtPathForwardSlash(char *str);
FILE *iRODSNt_fopen(const char *filename, const char *mode);
int iRODSNt_open(const char *filename,int oflag, int istextfile);
int iRODSNt_bopen(const char *filename,int oflag, int pmode);
int iRODSNt_bcreate(const char *filename);
int iRODSNt_unlink(char *filename);

int iRODSNt_stat(const char *filename,struct irodsntstat *stat_p);
int iRODSNt_mkdir(char *dir,int mode);

char *iRODSNt_gethome();
void iRODSNtGetUserPasswdInputInConsole(char *buf, char *prompt, int echo_input);

int getopt(int argc, char *const *argv, const char *shortopts);
long long atoll(const char *str);
void bzero(void *s, size_t n);
size_t irodsnt_strlen(const char *str);
void srandom(unsigned int seed);
int strcasecmp(const char *s1, const char *s2);

void irodsStrChangeChar(char* str,char from, char to);
void irodsNtPathBackSlash(char *str);
void irodsPath2NtPath(char *irodsPath, char *ntPath);

#ifdef  __cplusplus
}
#endif

#endif _IRODSNTUTIL_H_