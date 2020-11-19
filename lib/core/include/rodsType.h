#ifndef RODS_TYPE_H__
#define RODS_TYPE_H__

#include <sys/types.h>

#if defined(solaris_platform) || defined(aix_platform)
    #include <strings.h>
#endif

#include "rodsDef.h"

#if defined(osx_platform)
typedef int64_t rodsLong_t;
typedef u_int64_t rodsULong_t;
#elif defined(sgi_platform)
typedef __int64_t rodsLong_t;
typedef int64_t u_longlong_t;
#elif defined(linux_platform) || defined(alpha_platform)
typedef long long rodsLong_t;
typedef unsigned long long rodsULong_t;
#elif defined(windows_platform)
typedef unsigned int uint;
typedef __int64 rodsLong_t;
typedef unsigned __int64 rodsULong_t;
#else	/* windows_platform */
typedef long long rodsLong_t;
typedef unsigned long long rodsULong_t;
#endif	/* windows_platform */

typedef enum ObjectType {  /* object type */
    UNKNOWN_OBJ_T,
    DATA_OBJ_T,
    COLL_OBJ_T,
    UNKNOWN_FILE_T,
    LOCAL_FILE_T,
    LOCAL_DIR_T,
    NO_INPUT_T
} objType_t;

typedef enum ObjectStat {  /* object status */
    UNKNOWN_ST,
    NOT_EXIST_ST,
    EXIST_ST
} objStat_t;

typedef struct rodsStat {
    rodsLong_t   st_size;        /* file size */
    unsigned int st_dev;
    unsigned int st_ino;
    unsigned int st_mode;
    unsigned int st_nlink;
    unsigned int st_uid;
    unsigned int st_gid;
    unsigned int st_rdev;
    unsigned int st_atim;        /* time of last access */
    unsigned int st_mtim;        /* time of last mod */
    unsigned int st_ctim;        /* time of last status change */
    unsigned int st_blksize;     /* Optimal blocksize of FS */
    unsigned int st_blocks;      /* number of blocks */
} rodsStat_t;

#define DIR_LEN 	256

typedef struct rodsDirent {
    unsigned int d_offset;       /* offset after this entry */
    unsigned int d_ino;          /* inode number */
    unsigned int d_reclen;       /* length of this record */
    unsigned int d_namlen;       /* length of d_name */
    char         d_name[DIR_LEN];
} rodsDirent_t;

#endif	// RODS_TYPE_H__

