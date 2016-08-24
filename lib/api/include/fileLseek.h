#ifndef FILE_LSEEK_H__
#define FILE_LSEEK_H__

#include "rodsType.h"
#include "rcConnect.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \var fileLseekInp_t
 * \brief Input struct for rcDataObjLseek and rsFileLseek operations
 * \since 1.0
 *
 * \ingroup capi_input_data_structures
 *
 * \remark none
 *
 * \note
 * Elements of fileLseekInp_t:
 * \li int fileInx - the opened file descriptor from rcFileOpen or rcFileCreate.
 * \li rodsLong_t offset - the roffset
 * \li int whence - Similar to lseek of UNIX. Valid values are:
 *        \n SEEK_SET - The offset is set to offset bytes.
 *        \n SEEK_CUR - The offset is set to its current location plus
 *             offset bytes.
 *        \n SEEK_END - The offset is set to the size of the file plus
 *             offset bytes.
 *
 * \sa none
 */

typedef struct FileLseekInp {
    int fileInx;
    rodsLong_t offset;
    int whence;
} fileLseekInp_t;

/**
 * \var fileLseekOut_t
 * \brief Output struct for rcDataObjLseek and rsFileLseek operations
 * \since 1.0
 *
 * \remark none
 *
 * \note
 * Elements of fileLseekOut_t:
 * \li offset - the resulting offset location in bytes from the beginning
 *        of the file.
 *
 * \sa none
 */

typedef struct FileLseekOut {
    rodsLong_t offset;
} fileLseekOut_t;

#define fileLseekInp_PI "int fileInx; double offset; int whence;"
#define fileLseekOut_PI "double offset;"

#ifdef __cplusplus
extern "C"
#endif
int rcFileLseek( rcComm_t *conn, fileLseekInp_t *fileLseekInp, fileLseekOut_t **fileLseekOut );

#endif
