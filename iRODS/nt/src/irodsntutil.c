/*******************************************************************
 iRODSNtUtil.c
 Author: Bing Zhu
         San Diego Supercomputer Center
 Date of last modifictaion:  10-12-2005
 Modified: Charles Cowart
 Date of last modification: 11/15/2006
 *******************************************************************/
#include "iRODSNtutil.h"
#include <io.h>
#include <conio.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <errno.h>
#include <Windows.h>

#define IRODS_NT_SERVICE_MODE 0
#define IRODS_NT_CONSOLE_MODE 1

char iRODSNtServerHomeDir[1024];        /* the home dir, "....../dice" in the installed service */
int  nIrodsNtMode = IRODS_NT_SERVICE_MODE;
char *iRODSNtServerLogFileName = NULL;   /* pure file name */
char *iRODSNTServerLogDir = NULL;
char *iRODSNtServerConfigDir = NULL;


void iRODSNtAgentInit(int ac, char **av)
{
	iRODSNtServerCheckExecMode(ac, av);
	iRODSNtSetServerHomeDir(av[0]);
	if(nIrodsNtMode != IRODS_NT_CONSOLE_MODE)
	{
		char *log_fname = getenv("irodsLogFile");
		if(log_fname == NULL)
			return;
		iRODSNtServerLogFileName = strdup(log_fname);
	}
	if (startWinsock() != 0)
	{
		fprintf(stderr,"failed to call startWinsock().\n");
		exit(0);
	}
}

void iRODSNtServerCheckExecMode(int targc, char **targv)
{
	int i;
	for(i=0;i<targc;i++)
	{
		if(strcmp(targv[i], "console") == 0)
		{
			nIrodsNtMode = IRODS_NT_CONSOLE_MODE;
		}
	}
}

/*
int iRODSNtServerGetExecMode()
{
	return nIrodsNtMode;
}
*/

int iRODSNtServerRunningConsoleMode()
{
	if(nIrodsNtMode == IRODS_NT_CONSOLE_MODE)
		return 1;

	return 0;
}

void iRODSNtSetServerHomeDir(char *inExec)
{
	char tmpstr[1024];
	char *p;

	if(nIrodsNtMode == IRODS_NT_CONSOLE_MODE)
	{
		strcpy(iRODSNtServerHomeDir, ".\\..");
		return;
	}

	//strcpy(tmpstr,inExec);
	// we chang to use short name. The long name has some problem in XP.
	if(GetShortPathName(inExec, tmpstr, 1024) == 0)
	{
		fprintf(stderr, "GetShortPathName() failed. Program exits.\n");
		exit(0);
	}

	p = strrchr(tmpstr,'\\');
	if(p == NULL)
	{
		fprintf(stderr,"If you are running in sonsole mode. Please use the following syntax.\n");
		fprintf(stderr,"> irodsServer.exe console\n");
		exit(0);
	}

	p[0] = '\0';
	p = strrchr(tmpstr,'\\');
	if(p == NULL)
	{
		fprintf(stderr,"If you are running in sonsole mode. Please use the following syntax.\n");
		fprintf(stderr,"> irodsServer.exe console\n");
		exit(0);
	}
	p[0] = '\0';
	strcpy(iRODSNtServerHomeDir, tmpstr);
}

int iRODSNtGetServiceName(char *service_name)
{
	char irods_fname[2048];
	FILE *f;
	char buff[1024];
	int t;

	sprintf(irods_fname, "%s\\rodssnam.txt", iRODSNtServerHomeDir);
	f = iRODSNt_fopen(irods_fname, "r");
	if(f == NULL)
	{  /* failed to get service name. */
		fprintf(stderr,"The iRODS could not open service name file, %s.\n", irods_fname);
		return -1;
	}
    if(fgets(buff, 1024, f) == NULL)
	{
		fprintf(stderr,"The iRODS failed to get info from service name file, %s.\n", irods_fname);
		fclose(f);
		return -2;
	}

	t = strlen(buff);
	if(buff[t-1] == '\n')
		buff[t-1] = '\0';
	strcpy(service_name, buff);
	return 0;
}

void iRODSNtGetAgentExecutableWithPath(char *buf, char *agent_name)
{
	if(nIrodsNtMode == IRODS_NT_CONSOLE_MODE)
	{
		strcpy(buf, agent_name);
	}
	else
	{
		sprintf(buf, "%s\\bin\\%s", iRODSNtServerHomeDir, agent_name);
	}
}

char *iRODSNtGetServerConfigPath()
{
	char tmpstr[2048];

	if(iRODSNtServerConfigDir != NULL)
	{
		return iRODSNtServerConfigDir;
	}

	if(nIrodsNtMode == IRODS_NT_CONSOLE_MODE)
	{
		strcpy(tmpstr, "..\\config");
	}
	else
	{
		sprintf(tmpstr, "%s\\config", iRODSNtServerHomeDir);
	}
	iRODSNtServerConfigDir = _strdup(tmpstr);
	return iRODSNtServerConfigDir;
}

void iRODSNtAgentSetLogFilename(char *in_filename)
{
}

/* This function should be called only in irodsServer. */
void iRODSNtGetLogFilename(char *log_filename)
{
	if(iRODSNtServerLogFileName != NULL)
		strcpy(log_filename, iRODSNtServerLogFileName);
}

char *iRODSNtServerGetLogDir()
{
	char buff[1024];

	if(iRODSNTServerLogDir != NULL)
		return iRODSNTServerLogDir;

	sprintf(buff, "%s\\log", iRODSNtServerHomeDir);
	iRODSNTServerLogDir = strdup(buff);
}

void iRODSNtGetLogFilenameWithPath(char *logfilename_wp)
{
	SYSTEMTIME tm;
	char tmpfn[1024];
	
	if(nIrodsNtMode == IRODS_NT_CONSOLE_MODE)
	{
		logfilename_wp[0] = '\0';
		return;
	}

	if(iRODSNtServerLogFileName != NULL)
	{
		sprintf(tmpfn, "%s\\log\\%s", iRODSNtServerHomeDir, iRODSNtServerLogFileName);
		strcpy(logfilename_wp, tmpfn);
		return ;
	}

	GetSystemTime(&tm);
	sprintf(tmpfn,"rodsLog_%d_%d_%d.txt", tm.wMonth, tm.wDay, tm.wYear);
	iRODSNtServerLogFileName = _strdup(tmpfn);
	sprintf(tmpfn, "%s\\log\\%s", iRODSNtServerHomeDir, iRODSNtServerLogFileName);
	strcpy(logfilename_wp, tmpfn);
}

/* The function is used to convert unix path delimiter,slash, to
   windows path delimiter, back slash. 
 */
static void StrChangeChar(char* str,char from, char to)
{
	int n,i;

	if(str == NULL)
		return;

	n = strlen(str);

	for(i=0;i<n;i++)
	{
		if(str[i] == from)
			str[i] = to;
	}
}

void iRODSNtPathBackSlash(char *str)
{
	StrChangeChar(str,'/','\\');
}

void iRODSNtPathForwardSlash(char *str)
{
	StrChangeChar(str,'\\','/');
}

void iRODSPathToNtPath(char *ntpath,const char *srbpath)
{
	char buff[2048];

	if(strlen(srbpath) <= 3)
	{
		strcpy(buff,srbpath);
	}
	else if((srbpath[0] == '/')&&(srbpath[2] == ':'))  /* the path has form /D:/tete/... */
	{
		strcpy(buff,&(srbpath[1]));
	}
	else if((srbpath[0] == '\\')&&(srbpath[2] == ':'))
	{
		strcpy(buff,&(srbpath[1]));
	}
	else
	{
		strcpy(buff,srbpath);
	}

	iRODSNtPathBackSlash(buff);
	strcpy(ntpath,buff);
}

FILE *iRODSNt_fopen(const char *filename, const char *mode)
{
	char ntfp[2048];
	iRODSPathToNtPath(ntfp,filename);
	return fopen(ntfp,mode);
}

int iRODSNt_open(const char *filename,int oflag, int istextfile)
{
	int New_Oflag;
	char ntfp[2048];

	iRODSPathToNtPath(ntfp,filename);

	if(istextfile)
		New_Oflag = oflag;
	else
		New_Oflag = _O_BINARY | oflag;

	if(New_Oflag & _O_CREAT)
	{
		return _open(ntfp,New_Oflag,_S_IREAD|_S_IWRITE);
	}

	return _open(ntfp,New_Oflag);
}

/* open a file in binary mode. */
int iRODSNt_bopen(const char *filename,int oflag, int pmode)
{
	int New_Oflag;
	char ntfp[2048];

	iRODSPathToNtPath(ntfp,filename);

	New_Oflag = _O_BINARY | oflag;
	return _open(ntfp,New_Oflag,pmode);
}

/* create a file in binary mode */
int iRODSNt_bcreate(const char *filename)
{
	char ntfp[2048];

	iRODSPathToNtPath(ntfp,filename);

	return _open(ntfp,_O_RDWR|_O_CREAT|_O_EXCL|_O_BINARY, _S_IREAD|_S_IWRITE);
}

int iRODSNt_unlink(char *filename)
{
	char ntfp[2048];

	iRODSPathToNtPath(ntfp,filename);

	return _unlink(ntfp);
}

int iRODSNt_stat(const char *filename,struct irodsntstat *stat_p)
{
        char ntfp[2048];
        iRODSPathToNtPath(ntfp,filename);
        return _stat64(ntfp,stat_p);    /* _stat */
}

int iRODSNt_mkdir(char *dir,int mode)
{
        char ntfp[2048];
        iRODSPathToNtPath(ntfp,dir);
        return _mkdir(ntfp);
}

/* the caller needs to free the memory. */
char *iRODSNt_gethome()
{
	char *s1, *s2;
	char tmpstr[1024];
	s1 = getenv("HOMEDRIVE");
	s2 = getenv("HOMEPATH");

	if((s1==NULL)||(strlen(s1)==0))
		return NULL;

	if((s2==NULL)||(strlen(s2)==0))
		return NULL;

	sprintf(tmpstr, "%s%s", s1, s2);
	return strdup(tmpstr);
}

/* The function is used in Windows console app, especially S-commands. */
void iRODSNtGetUserPasswdInputInConsole(char *buf, char *prompt, int echo_input)
{
   char *p = buf;
   char c;
   char star='*';
   char whitechar = ' ';

   if((prompt != NULL) && (strlen(prompt) > 0))
   {
      printf("%s", prompt);
      fflush(stdout);
   }

   while(1)
   {
      c = _getch();

      if(c == 8)   /* a backspace, we currently ignore it. i.e. treat it as doing nothing. User can alway re-do it. */
      {
		  if(p > buf)
		  {
			  --p;
			  if(echo_input)
			  {
					_putch(c);
					_putch(whitechar);
					_putch(c);
		      }
		  }
      }
      else if(c == 13)  /* 13 is a return char */
      {
         _putch(c);
         break;
      }
      else
      {
         /* _putch(star); */
		  
         p[0] = c;
         ++p;
		 if(echo_input)
		 {
			  _putch(c);
		 }
      }

      /* extra protection */
      if((c == '\n') || (c == '\f') || (c == 13))
        break;

   }

   p[0] = '\0';

   printf("\n");
}

long long atoll(const char *str)
{
	return _atoi64(str);
}

void bzero(void *s, size_t n)
{
	memset((char *)s, 0, n);
}

size_t irodsnt_strlen(const char *str)
{
	if(str == NULL)
		return 0;

	return strlen(str);
}

void srandom(unsigned int seed)
{
	srand(seed);
}

int strcasecmp(const char *s1, const char *s2)
{
        return _stricmp(s1,s2);
}

void irodsStrChangeChar(char* str,char from, char to)
{
        int n,i;

        if(str == NULL)
                return;

        n = strlen(str);

        for(i=0;i<n;i++)
        {
                if(str[i] == from)
                        str[i] = to;
        }
}
void irodsNtPathBackSlash(char *str)
{
	irodsStrChangeChar(str,'/','\\');
}

void irodsPath2NtPath(char *irodsPath, char *ntPath)
{
	char buff[2048];
	if(irodsnt_strlen(irodsPath) <= 3)
    {
		strcpy(buff,irodsPath);
    }
    else if((irodsPath[0] == '/')&&(irodsPath[2] == ':'))  /* the path has form /D:/tete/... */
    {
		strcpy(buff,&(irodsPath[1]));
    }
    else if((irodsPath[0] == '\\')&&(irodsPath[2] == ':'))
    {
		strcpy(buff,&(irodsPath[1]));
    }
    else
    {
		strcpy(buff,irodsPath);
    }

    irodsNtPathBackSlash(buff);
    strcpy(ntPath,buff);
}