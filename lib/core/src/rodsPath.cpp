#include "irods/rodsPath.h"
#include "irods/miscUtil.h"
#include "irods/rcMisc.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsLog.h"
#ifdef windows_platform
#include "irodsntutil.hpp"
#endif

#include "irods/filesystem/path.hpp"

#include <string>
#include <string_view>
#include <map>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

/* parseRodsPathStr - This is similar to parseRodsPath except the
 * input and output are char string inPath and outPath
 */

int
parseRodsPathStr( const char *inPath, rodsEnv *myRodsEnv, char *outPath ) {
    int status;
    rodsPath_t rodsPath;

    if ( inPath == NULL) {
        rodsLog( LOG_ERROR,
                 "parseRodsPathStr: NULL inPath input" );
        return USER__NULL_INPUT_ERR;
    } else if ( strnlen(inPath, MAX_NAME_LEN) >= MAX_NAME_LEN - 1) {
        rodsLog( LOG_ERROR,
                 "parseRodsPath: parsing error - path too long" );
        return USER_PATH_EXCEEDS_MAX;
    }
    rstrcpy( rodsPath.inPath, inPath, MAX_NAME_LEN );

    status = parseRodsPath( &rodsPath, myRodsEnv );

    if ( status < 0 ) {
        return status;
    }

    rstrcpy( outPath, rodsPath.outPath, MAX_NAME_LEN );

    return status;
}

/* parseRodsPath - Parse a rods path into full rods path taking care of
 * all the "/../", "/./" and "." entries in the path.
 * The input is taken from rodsPath->inPath.
 * The output path is written in rodsPath->outPath.
 *
 */

int
parseRodsPath( rodsPath_t *rodsPath, rodsEnv *myRodsEnv ) {
    int len;
    char *tmpPtr1, *tmpPtr2;
    char tmpStr[MAX_NAME_LEN];


    if ( rodsPath == NULL ) {
        fprintf( stderr, "parseRodsPath: NULL rodsPath input\n" );
        return USER__NULL_INPUT_ERR;
    }

    rodsPath->objType = UNKNOWN_OBJ_T;
    rodsPath->objState = UNKNOWN_ST;

    if ( myRodsEnv == NULL && rodsPath->inPath[0] != '/' ) {
        fprintf( stderr, "parseRodsPath: NULL myRodsEnv input\n" );
        return USER__NULL_INPUT_ERR;
    }

    len = strlen( rodsPath->inPath );

    if ( len == 0 ) {
        /* just copy rodsCwd */
        rstrcpy( rodsPath->outPath, myRodsEnv->rodsCwd, MAX_NAME_LEN );
        rodsPath->objType = COLL_OBJ_T;
        return 0;
    }
    else if ( strcmp( rodsPath->inPath, "." ) == 0 ||
              strcmp( rodsPath->inPath, "./" ) == 0 ) {
        /* '.' or './' */
        rstrcpy( rodsPath->outPath, myRodsEnv->rodsCwd, MAX_NAME_LEN );
        rodsPath->objType = COLL_OBJ_T;
        return 0;
    }
    else if ( strcmp( rodsPath->inPath, "~" ) == 0 ||
              strcmp( rodsPath->inPath, "~/" ) == 0) {
        /* ~ or ~/ */
        rstrcpy( rodsPath->outPath, myRodsEnv->rodsHome, MAX_NAME_LEN );
        rodsPath->objType = COLL_OBJ_T;
        return 0;
    }
    else if (rodsPath->inPath[0] == '~') {
        if ( rodsPath->inPath[1] == '/' ) {
            snprintf( rodsPath->outPath, MAX_NAME_LEN, "%s/%s",
                      myRodsEnv->rodsHome, rodsPath->inPath + 2 );
        }
        else {
            // treat it like a relative path
            snprintf( rodsPath->outPath, MAX_NAME_LEN, "%s/%s",
                      myRodsEnv->rodsCwd, rodsPath->inPath );
        }
    }
    else if ( rodsPath->inPath[0] == '/' ) {
        /* full path */
        rstrcpy( rodsPath->outPath, rodsPath->inPath, MAX_NAME_LEN );
    }
    else {
        /* a relative path */
        snprintf( rodsPath->outPath, MAX_NAME_LEN, "%s/%s",
                  myRodsEnv->rodsCwd, rodsPath->inPath );
    }

    /* take out any "//" */

    while ( ( tmpPtr1 = strstr( rodsPath->outPath, "//" ) ) != NULL ) {
        //    rstrcpy (tmpPtr1 + 1, tmpPtr1 + 2, MAX_NAME_LEN);
        rstrcpy( tmpStr, tmpPtr1 + 2, MAX_NAME_LEN );
        rstrcpy( tmpPtr1 + 1, tmpStr, MAX_NAME_LEN );
    }

    /* take out any "/./" */

    while ( ( tmpPtr1 = strstr( rodsPath->outPath, "/./" ) ) != NULL ) {
        //    rstrcpy (tmpPtr1 + 1, tmpPtr1 + 3, MAX_NAME_LEN);
        rstrcpy( tmpStr, tmpPtr1 + 3, MAX_NAME_LEN );
        rstrcpy( tmpPtr1 + 1, tmpStr, MAX_NAME_LEN );
    }

    /* take out any /../ */

    while ( ( tmpPtr1 = strstr( rodsPath->outPath, "/../" ) ) != NULL ) {
        /* go back */
        tmpPtr2 = tmpPtr1 - 1;
        while ( *tmpPtr2 != '/' ) {
            tmpPtr2 --;
            if ( tmpPtr2 < rodsPath->outPath ) {
                rodsLog( LOG_ERROR,
                         "parseRodsPath: parsing error for %s",
                         rodsPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
        }
        //    rstrcpy (tmpPtr2 + 1, tmpPtr1 + 4, MAX_NAME_LEN);
        rstrcpy( tmpStr, tmpPtr1 + 4, MAX_NAME_LEN );
        rstrcpy( tmpPtr2 + 1, tmpStr, MAX_NAME_LEN );
    }

    /* handle "/.", "/.." and "/" at the end */

    len = strlen( rodsPath->outPath );

    tmpPtr1 = rodsPath->outPath + len;

    if ( ( tmpPtr2 = strstr( tmpPtr1 - 3, "/.." ) ) != NULL ) {
        /* go back */
        tmpPtr2 -= 1;
        while ( *tmpPtr2 != '/' ) {
            tmpPtr2 --;
            if ( tmpPtr2 < rodsPath->outPath ) {
                rodsLog( LOG_ERROR,
                         "parseRodsPath: parsing error for %s",
                         rodsPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
        }
        *tmpPtr2 = '\0';
        if ( tmpPtr2 == rodsPath->outPath ) { /* nothing, special case */
            *tmpPtr2++ = '/'; /* root */
            *tmpPtr2 = '\0';
        }
        rodsPath->objType = COLL_OBJ_T;
        if ( strnlen(rodsPath->outPath, MAX_PATH_ALLOWED) >= MAX_PATH_ALLOWED - 1) {
            rodsLog( LOG_ERROR,
                     "parseRodsPath: parsing error - path too long" );
            return USER_PATH_EXCEEDS_MAX;
        }
        return 0;
    }

    /* take out "/." */

    if ( ( tmpPtr2 = strstr( tmpPtr1 - 2, "/." ) ) != NULL ) {
        *tmpPtr2 = '\0';
        rodsPath->objType = COLL_OBJ_T;
        if ( strnlen(rodsPath->outPath, MAX_PATH_ALLOWED) >= MAX_PATH_ALLOWED - 1) {
            rodsLog( LOG_ERROR,
                     "parseRodsPath: parsing error - path too long" );
            return USER_PATH_EXCEEDS_MAX;
        }
        return 0;
    }

    if ( *( tmpPtr1 - 1 ) == '/' && len > 1 ) {
        *( tmpPtr1 - 1 ) = '\0';
        rodsPath->objType = COLL_OBJ_T;
        if ( strnlen(rodsPath->outPath, MAX_PATH_ALLOWED) >= MAX_PATH_ALLOWED - 1) {
            rodsLog( LOG_ERROR,
                     "parseRodsPath: parsing error - path too long" );
            return USER_PATH_EXCEEDS_MAX;
        }
        return 0;
    }
    if ( strnlen(rodsPath->outPath, MAX_PATH_ALLOWED) >= MAX_PATH_ALLOWED - 1) {
        rodsLog( LOG_ERROR,
                 "parseRodsPath: parsing error - path too long" );
        return USER_PATH_EXCEEDS_MAX;
    }
    return 0;
}

/* parseLocalPath - Parse a local path into full path taking care of
 * all the "/../", "/./" and "." entries in the path.
 * The input is taken from rodsPath->inPath.
 * The output path is written in rodsPath->outPath.
 * In addition, a stat is done on the rodsPath->outPath to determine the
 * objType and size of the file.
 */

int
parseLocalPath( rodsPath_t *rodsPath ) {
    if ( rodsPath == NULL ) {
        fprintf( stderr, "parseLocalPath: NULL rodsPath input\n" );
        return USER__NULL_INPUT_ERR;
    }

    if ( strlen( rodsPath->inPath ) == 0 ) {
        rstrcpy( rodsPath->outPath, ".", MAX_NAME_LEN );
    }
    else {
        rstrcpy( rodsPath->outPath, rodsPath->inPath, MAX_NAME_LEN );
    }

    return getFileType( rodsPath );
}

int
getFileType( rodsPath_t *rodsPath ) {
    boost::filesystem::path p( rodsPath->outPath );

    try {
        if ( !exists( p ) ) {
            rodsPath->objType = UNKNOWN_FILE_T;
            rodsPath->objState = NOT_EXIST_ST;
            return NOT_EXIST_ST;
        }
        else if ( is_regular_file( p ) ) {
            rodsPath->objType = LOCAL_FILE_T;
            rodsPath->objState = EXIST_ST;
            rodsPath->size = file_size( p );
        }
        else if ( is_directory( p ) ) {
            rodsPath->objType = LOCAL_DIR_T;
            rodsPath->objState = EXIST_ST;
        }
    }
    catch ( const boost::filesystem::filesystem_error& e ) {
        fprintf( stderr, "%s\n", e.what() );
        return SYS_NO_PATH_PERMISSION;
    }


    return rodsPath->objType;
}

int
addSrcInPath( rodsPathInp_t *rodsPathInp, const char *inPath ) {
    rodsPath_t *newSrcPath, *newTargPath;
    int newNumSrc;

    if ( rodsPathInp == NULL || inPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "addSrcInPath: NULL input" );
        return USER__NULL_INPUT_ERR;
    }

    // Issue #3658 - This section removes trailing slashes from the 
    // incoming directory path in an invoking icommand (e.g. ireg).
    //
    // TODO:  This is a localized fix that addresses any trailing slashes
    // in inPath. Any future refactoring of the code that deals with paths
    // would most probably take care of this issue, and the code segment
    // below would/should then be removed.

    char tmpStr[MAX_NAME_LEN];

    rstrcpy( tmpStr, inPath, MAX_NAME_LEN );
    size_t len = strlen(tmpStr);
    for (size_t i = len-1; i > 0 && tmpStr[i] == '/'; i--) {
        tmpStr[i] = '\0';
    }
    const char *modInPath = const_cast<const char *>(tmpStr);   // end of 3658 fix

    if ( ( rodsPathInp->numSrc % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newNumSrc = rodsPathInp->numSrc + PTR_ARRAY_MALLOC_LEN;
        newSrcPath = ( rodsPath_t * ) malloc( newNumSrc * sizeof( rodsPath_t ) );
        newTargPath = ( rodsPath_t * ) malloc( newNumSrc * sizeof( rodsPath_t ) );
        memset( ( void * ) newSrcPath, 0, newNumSrc * sizeof( rodsPath_t ) );
        memset( ( void * ) newTargPath, 0, newNumSrc * sizeof( rodsPath_t ) );
        if ( rodsPathInp->numSrc > 0 ) {
            memcpy( newSrcPath, rodsPathInp->srcPath,
                    rodsPathInp->numSrc * sizeof( rodsPath_t ) );
            memcpy( newTargPath, rodsPathInp->targPath,
                    rodsPathInp->numSrc * sizeof( rodsPath_t ) );
            free( rodsPathInp->srcPath );
            free( rodsPathInp->targPath );
        }
        rodsPathInp->srcPath = newSrcPath;
        rodsPathInp->targPath = newTargPath;
    }
    else {
        newSrcPath = rodsPathInp->srcPath;
    }
    rstrcpy( newSrcPath[rodsPathInp->numSrc].inPath, modInPath, MAX_NAME_LEN );
    rodsPathInp->numSrc++;

    return 0;
}

/* parseCmdLinePath - parse the iCommand line path input. It can parse
 * path input of the form:
 * 1) srcPath1, srcPath2 ... destPath
 * 2) srcPath1, srcPath2 ...
 *
 * The source and destination paths can be irodsPath (data object) or
 * local path.
 * srcFileType/destFileType = UNKNOWN_OBJ_T means the path is a irodsPath.
 * srcFileType/destFileType = UNKNOWN_FILE_T means the path is a local path
 * destFileType = NULL_T means there is no destPath.
 *
 * argc and argv should be the usual values directly from the main ()
 * routine.
 * optind - directly from the getopt call if the path inputs are directly
 * behind the input options or equal to (optind + n) where
 * where n is number of arguments before the path inputs.
 *
 * rodsPathInp - The output struct.
 * typedef struct RodsPathInp {
 *     int numSrc;
 *     rodsPath_t *srcPath;         // pointer to an array of rodsPath_t
 *     rodsPath_t *destPath;
 * } rodsPathInp_t;
 *
 * typedef struct RodsPath {
 *    objType_t objType;
 *    rodsLong_t size;
 *    char inPath[MAX_NAME_LEN];
 *    char outPath[MAX_NAME_LEN];
 * } rodsPath_t;
 *
 * inPath contains the input value.
 * outPath contains the parsed output path.
 *
 * Note that parseCmdLinePath only parses the path. It does not resolve the
 * dependency of the destination path to the source path.
 * e.g., "iput foo1 ."
 * The resolveRodsTarget() should be called to resolve the dependency.
 *
 */

int
parseCmdLinePath( int argc, char **argv, int optInd, rodsEnv *myRodsEnv,
                  int srcFileType, int destFileType, int flag, rodsPathInp_t *rodsPathInp ) {
    int nInput;
    int i, status;
    int numSrc;

    nInput = argc - optInd;

    if ( rodsPathInp == NULL ) {
        rodsLog( LOG_ERROR, "parseCmdLinePath: NULL rodsPathInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( rodsPathInp, 0, sizeof( rodsPathInp_t ) );

    if ( nInput <= 0 ) {
        if ( ( flag & ALLOW_NO_SRC_FLAG ) == 0 ) {
            return USER__NULL_INPUT_ERR;
        }
        else {
            numSrc = 1;
        }
    }
    else if ( nInput == 1 ) {
        numSrc = 1;
    }
    else if ( destFileType == NO_INPUT_T ) {            /* no dest input */
        numSrc = nInput;
    }
    else {
        numSrc = nInput - 1;
    }

    for ( i = 0; i < numSrc; i++ ) {
        if ( nInput <= 0 ) {
            /* just add cwd */
            addSrcInPath( rodsPathInp, "." );
        }
        else {
            addSrcInPath( rodsPathInp, argv[optInd + i] );
        }

        if ( srcFileType <= COLL_OBJ_T ) {
            status = parseRodsPath( &rodsPathInp->srcPath[i], myRodsEnv );
            if (status == 0) {
                auto* escaped_path = escape_path(rodsPathInp->srcPath->outPath);
                rstrcpy(rodsPathInp->srcPath->outPath, escaped_path, MAX_NAME_LEN);
                std::free(escaped_path);
            }
        }
        else {
            status = parseLocalPath( &rodsPathInp->srcPath[i] );
        }

        if ( status < 0 ) {
            return status;
        }
    }

    if ( destFileType != NO_INPUT_T ) {
        rodsPathInp->destPath = ( rodsPath_t* )malloc( sizeof( rodsPath_t ) );
        memset( rodsPathInp->destPath, 0, sizeof( rodsPath_t ) );
        if ( nInput > 1 ) {
            rstrcpy( rodsPathInp->destPath->inPath, argv[argc - 1],
                     MAX_NAME_LEN );
        }
        else {
            rstrcpy( rodsPathInp->destPath->inPath, ".", MAX_NAME_LEN );
        }

        if ( destFileType <= COLL_OBJ_T ) {
            status = parseRodsPath( rodsPathInp->destPath, myRodsEnv );
            if (status == 0) {
                auto* escaped_path = escape_path(rodsPathInp->destPath->outPath);
                rstrcpy(rodsPathInp->destPath->outPath, escaped_path, MAX_NAME_LEN);
                std::free(escaped_path);
            }
        }
        else {
            status = parseLocalPath( rodsPathInp->destPath );
        }
    }

    return status;
}

int getLastPathElement(char* _logical_path, char* _last_path_element)
{
    if (!_logical_path) {
        *_last_path_element = '\0';
        return 0;
    }

    namespace fs = irods::experimental::filesystem;

    if (std::string_view sv = _logical_path; fs::path::preferred_separator == sv.back()) {
        sv = sv.substr(0, sv.size() - 1);
        rstrcpy(_last_path_element, std::string{sv}.data(), MAX_NAME_LEN);
        return 0;
    }

    auto object_name = fs::path{_logical_path}.object_name();

    if ("." == object_name || ".." == object_name) {
        object_name.remove_object_name();
        *_last_path_element = '\0';
        return 0;
    }

    std::string_view sv = object_name.c_str();

    if (fs::path::preferred_separator == sv.back()) {
        sv = sv.substr(0, sv.size() - 1);
    }

    rstrcpy(_last_path_element, std::string{sv}.data(), MAX_NAME_LEN);

    return 0;
}

void
clearRodsPath( rodsPath_t *rodsPath ) {
    if ( rodsPath == NULL ) {
        return;
    }

    if ( rodsPath->rodsObjStat == NULL ) {
        return;
    }

    freeRodsObjStat( rodsPath->rodsObjStat );

    memset( rodsPath, 0, sizeof( rodsPath_t ) );

    return;
}

char* escape_path(const char* _path)
{
    static const std::map<char, std::string> special_char_mappings{
        {'\f', "\\f"}
    };

    std::string path = _path;
    std::string escaped_path;
    const auto end = std::end(special_char_mappings);

    for (auto&& c : path) {
        if (const auto iter = special_char_mappings.find(c); end != iter) {
            escaped_path += iter->second;
        }
        else {
            escaped_path += c;
        }
    }

    const auto size = escaped_path.size() + 1;
    char* escaped_path_c_str = static_cast<char*>(std::malloc(sizeof(char) * size));
    std::memset(escaped_path_c_str, '\0', size);
    std::strcpy(escaped_path_c_str, escaped_path.c_str());

    return escaped_path_c_str;
}

int has_trailing_path_separator(const char* path)
{
    if (std::string_view p = path; !p.empty()) {
        namespace fs = irods::experimental::filesystem;
        return *std::rbegin(p) == fs::path::preferred_separator;
    }

    return 0;
}

void remove_trailing_path_separators(char* path)
{
    if (!path) {
        return;
    }

    namespace fs = irods::experimental::filesystem;

    const fs::path p = path;

    // If the last path element is not empty, then the path does not
    // end with trailing slashes.
    if (!std::rbegin(p)->empty()) {
        return;
    }

    fs::path new_p;

    for (auto iter = std::begin(p), end = std::prev(std::end(p)); iter != end; ++iter) {
        new_p /= *iter;
    }

    std::strcpy(path, new_p.c_str());
}

int has_prefix(const char* path, const char* prefix)
{
    namespace fs = irods::experimental::filesystem;

    const fs::path parent = prefix;
    const fs::path child = path;

    if (parent.empty() || parent == child) {
        return false;
    }

    auto p_iter = std::begin(parent);
    auto p_last = std::end(parent);
    auto c_iter = std::begin(child);
    auto c_last = std::end(child);

    // Paths that end with a trailing slash will have an empty path element
    // just before the end iterator. We ignore this empty path element by making
    // the end iterator point to the empty path element.
    if (auto tmp = std::prev(p_last); "" == *tmp) {
        p_last = tmp;
    }

    for (; p_iter != p_last && c_iter != c_last && *p_iter == *c_iter; ++p_iter, ++c_iter);

    return (p_iter == p_last);
}

