#ifndef RODS_PATH_H__
#define RODS_PATH_H__

#include "rodsDef.h"
#include "rods.h"
#include "getRodsEnv.h"
#include "rodsType.h"
#include "objStat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STDOUT_FILE_NAME	"-"	/* pipe to stdout */

typedef struct RodsPath {
    objType_t objType;
    objStat_t objState;
    rodsLong_t size;
    uint objMode;
    char inPath[MAX_NAME_LEN];	 /* input from command line */
    char outPath[MAX_NAME_LEN];	 /* the path after parsing the inPath */
    char dataId[NAME_LEN];
    char chksum[NAME_LEN];
    rodsObjStat_t *rodsObjStat;
} rodsPath_t;

/* This is the struct for a command line path input. Normally it contains
 * one or more source input paths and 0 or 1 destination paths */

typedef struct RodsPathInp {
    int numSrc;
    rodsPath_t *srcPath;	/* pointr to an array of rodsPath_t */
    rodsPath_t *destPath;
    rodsPath_t *targPath;	/* This is a target path for a
                                  * source/destination type command */
    int resolved;
} rodsPathInp_t;

/* definition for flag in parseCmdLinePath */

#define	ALLOW_NO_SRC_FLAG	0x1

int
parseRodsPath( rodsPath_t *rodsPath, rodsEnv *myRodsEnv );
int
parseRodsPathStr( const char *inPath, rodsEnv *myRodsEnv, char *outPath );
int
addSrcInPath( rodsPathInp_t *rodsPathInp, const char *inPath );
int
parseLocalPath( rodsPath_t *rodsPath );
int
parseCmdLinePath( int argc, char **argv, int optInd, rodsEnv *myRodsEnv,
                  int srcFileType, int destFileType, int flag, rodsPathInp_t *rodsPathInp );

int
getLastPathElement( char *inPath, char *lastElement );

int
getFileType( rodsPath_t *rodsPath );
void
clearRodsPath( rodsPath_t *rodsPath );

// Returns a new path with the following special characters escaped:
//   - '\f'
//
// The character array returned is dynamically allocated. The caller is expected
// to deallocate this memory using "free".
char* escape_path(const char* _path);

// Returns a non-zero value if the path ends with a trailing path separator ("/"), else zero.
int has_trailing_path_separator(const char* path);

// Removes trailing slashes from path in-place.
void remove_trailing_path_separators(char* path);

/// Returns whether \p path starts with \p prefix.
///
/// \p path and \p prefix are expected to be null-terminated strings.
/// The behavior is undefined if either string is null or not null-terminated.
///
/// \since 4.2.8
///
/// \param[in] path   The path to search.
/// \param[in] prefix The path to look for. Trailing slashes are ignored.
///
/// \return An interger value.
/// \retval non-zero If \p path starts with \p prefix.
/// \retval 0        If \p path does not start with \p prefix or \p prefix is an empty string.
int has_prefix(const char* path, const char* prefix);
#ifdef __cplusplus
}
#endif

#endif	// RODS_PATH_H__
