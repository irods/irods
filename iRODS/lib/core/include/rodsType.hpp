/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rodsType.h - rods data type header files
 */



#ifndef RODS_TYPE_H
#define RODS_TYPE_H

#include <sys/types.h>
#if defined(solaris_platform) || defined(aix_platform)
#include <strings.h>
#endif
#include "rodsDef.h"

#if defined(osx_platform)
typedef int64_t rodsLong_t;
typedef u_int64_t rodsULong_t;
#else	/* osx_platform */
#if defined(sgi_platform)
typedef __int64_t rodsLong_t;
typedef int64_t u_longlong_t;
#else	/* sgi_platform */
#if defined(linux_platform) || defined(alpha_platform)
typedef long long rodsLong_t;
typedef unsigned long long rodsULong_t;
#else	/* linux_platform */
#if defined(windows_platform)
typedef unsigned int uint;
typedef __int64 rodsLong_t;
typedef unsigned __int64 rodsULong_t;
#else	/* windows_platform */
typedef long long rodsLong_t;
typedef unsigned long long rodsULong_t;
#endif	/* windows_platform */
#endif	/* linux_platform */
#endif	/* sgi_platform */
#endif	/* osx_platform */

#ifdef ADDR_64BITS
typedef rodsULong_t addrInt_t;
#else
typedef unsigned int addrInt_t;
#endif

typedef enum {  /* object type */
    UNKNOWN_OBJ_T,
    DATA_OBJ_T,
    COLL_OBJ_T,
    UNKNOWN_FILE_T,
    LOCAL_FILE_T,
    LOCAL_DIR_T,
    NO_INPUT_T
} objType_t;

typedef enum {  /* object status */
    UNKNOWN_ST,
    NOT_EXIST_ST,
    EXIST_ST
} objStat_t;

typedef struct rodsStat {
    rodsLong_t          st_size;        /* file size */
    unsigned int        st_dev;
    unsigned int        st_ino;
    unsigned int        st_mode;
    unsigned int        st_nlink;
    unsigned int        st_uid;
    unsigned int        st_gid;
    unsigned int        st_rdev;
    unsigned int        st_atim;        /* time of last access */
    unsigned int        st_mtim;        /* time of last mod */
    unsigned int        st_ctim;        /* time of last status change */
    unsigned int        st_blksize;     /* Optimal blocksize of FS */
    unsigned int        st_blocks;      /* number of blocks */
} rodsStat_t;

#define DIR_LEN 	256

typedef struct rodsDirent {
        unsigned int    d_offset;       /* offset after this entry */
        unsigned int    d_ino;          /* inode number */
        unsigned int    d_reclen;       /* length of this record */
        unsigned int    d_namlen;       /* length of d_name */
        char            d_name[DIR_LEN];
} rodsDirent_t;

#endif	/* RODS_TYPE_H */
