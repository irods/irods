/*****************************************************************************
 Unix2Nt.h
 *****************************************************************************/


#ifndef __UNIX2NT_h__
#define __UNIX2NT_h__

#ifdef _WIN32

#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <process.h>

#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif

#ifndef O_WRONLY
#define O_WRONLY _O_WRONLY
#endif

#ifndef O_RDWR
#define O_RDWR _O_RDWR
#endif

#ifndef O_APPEND
#define O_APPEND _O_APPEND
#endif

#ifndef O_CREAT
#define O_CREAT _O_CREAT
#endif

#ifndef O_TRUNC
#define O_TRUNC _O_TRUNC
#endif

#ifndef O_EXCL
#define O_EXCL _O_EXCL
#endif


/*Windows NT does not support soft links. S_IFLNK is defined such that if used
the conditional can fail properly.*/
#define _S_IFLNK 0xFFFF
#define S_IFLNK	_S_IFLNK


/* 
#define open _open 
#define close _close
#define read _read
#define write _write
#define mkdir(a,b) _mkdir(a)
#define unlink _unlink
#define rmdir _rmdir
#define create _open
#define dup _dup
#define fdopen _fdopen
#define getpid _getpid
*/



typedef unsigned short mode_t;
typedef long ssize_t;
typedef	unsigned int	u_int;


#define	S_IRWXU 	0000700	/* rwx, owner */
#define		S_IRUSR	0000400	/* read permission, owner */
#define		S_IWUSR	0000200	/* write permission, owner */
#define		S_IXUSR	0000100	/* execute/search permission, owner */
#define	S_IRWXG		0000070	/* rwx, group */
#define		S_IRGRP	0000040	/* read permission, group */
#define		S_IWGRP	0000020	/* write permission, grougroup */
#define		S_IXGRP	0000010	/* execute/search permission, group */
#define	S_IRWXO		0000007	/* rwx, other */
#define		S_IROTH	0000004	/* read permission, other */
#define		S_IWOTH	0000002	/* write permission, other */
#define		S_IXOTH	0000001	/* execute/search permission, other */



/* for locking */
#define F_DUPFD     0   /* Duplicate fildes */
#define F_GETFD     1   /* Get fildes flags (close on exec) */
#define F_SETFD     2   /* Set fildes flags (close on exec) */
#define F_GETFL     3   /* Get file flags */
#define F_SETFL     4   /* Set file flags */
#ifndef _POSIX_SOURCE
#define F_GETOWN    5   /* Get owner - for ASYNC */
#define F_SETOWN    6   /* Set owner - for ASYNC */
#endif  /* !_POSIX_SOURCE */
#define F_GETLK     7   /* Get record-locking information */
#define F_SETLK     8   /* Set or Clear a record-lock (Non-Blocking) */
#define F_SETLKW    9   /* Set or Clear a record-lock (Blocking) */
#ifndef _POSIX_SOURCE
#define F_RGETLK    10  /* Test a remote lock to see if it is blocked */
#define F_RSETLK    11  /* Set or unlock a remote lock */
#define F_CNVT      12  /* Convert a fhandle to an open fd */
#define F_RSETLKW   13  /* Set or Clear remote record-lock(Blocking) */
#endif  /* !_POSIX_SOURCE */


/* fcntl(2) flags (l_type field of flock structure) */
#define F_RDLCK     1   /* read lock */
#define F_WRLCK     2   /* write lock */
#define F_UNLCK     3   /* remove lock(s) */
#ifndef _POSIX_SOURCE
#define F_UNLKSYS   4   /* remove remote locks for a given system */
#endif  /* !_POSIX_SOURCE */

#ifndef off_t
typedef long    off_t;
#endif

#ifndef pid_t
typedef int pid_t;
#endif

typedef struct _srb_w_flock_ {
          short     l_type;
          short     l_whence;
          off_t     l_start;
          off_t     l_len;          /* len == 0 means until end of file */
          long      l_sysid;
          pid_t     l_pid;
          long      pad[4];         /* reserve area */
} flock_t;

struct timezone 
{
  int  tz_minuteswest; 
  int  tz_dsttime;  
};

#ifdef  __cplusplus
extern "C" {
#endif
/*
int gettimeofday(struct timeval *tp, void* timezone);
*/
int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef  __cplusplus
}
#endif

#endif /* _WIN32 */



#endif /* __UNIX2NT_h__ */
