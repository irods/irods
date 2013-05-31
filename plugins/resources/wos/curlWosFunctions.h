#ifndef CURL_WOS_FUNCTIONS_H
#define CURL_WOS_FUNCTIONS_H
/**
 * @file
 * @author  Howard Lander <howard@renci.org>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This software is open source.
 *
 * Renaissance Computing Institute,
 * (A Joint Institute between the University of North Carolina at Chapel Hill,
 * North Carolina State University, and Duke University)
 * http://www.renci.org
 *
 * For questions, comments please contact software@renci.org
 *
 * @section DESCRIPTION
 *
 * This file contains the definitions for the c code to exercise the DDN WOS 
 * rest interface.  The code uses libcurl to access the interface.  The code
 * currently supports the get, put and delete operations.
 */

/** @name WOS Headers
 * These defines are for the headers used in the WOS rest interface
 */
///@{
#define WOS_CONTENT_TYPE_HEADER "content-type: application/octet-stream"
#define WOS_CONTENT_LENGTH_HEADER "content-length: 0"
#define WOS_CONTENT_LENGTH_PUT_HEADER "content-length: "
#define WOS_OID_HEADER "x-ddn-oid:"
#define WOS_STATUS_HEADER "x-ddn-status:"
#define WOS_META_HEADER "x-ddn-meta:"
#define WOS_POLICY_HEADER "x-ddn-policy:"
#define WOS_LENGTH_HEADER "x-ddn-length:"
///@}

/** @name WOS Interface codes
 * These defines map directly to the WOS rest interface status codes
 */
///@{
#define WOS_OK 0
#define WOS_NO_NODE_FOR_POLICY 200
#define WOS_NO_NODE_FOR_OBJECT 201
#define WOS_UNKNOWN_POLICY_NAME 202
#define WOS_INTERNAL_ERROR 203
#define WOS_INVALID_OBJID 205
#define WOS_NO_SPACE 206
#define WOS_OBJ_NOT_FOUND 207
#define WOS_OBJ_CORRUPTED 208
#define WOS_FS_CORRUPTED 209
#define WOS_POLICY_NOT_SUPPORTED 210
#define WOS_IOERR 211
#define WOS_INVALID_OBJECT_SIZE 212
#define WOS_MISSING_OBJECT 213
#define WOS_TEMPORARILY_NOT_SUPPORTED 214
///@}

/** @name WOS operations.
 * The set of supported operations. These are added to the URL in the
 * curl code.
 */
///@{
#define WOS_COMMAND_GET "/cmd/get"
#define WOS_COMMAND_PUT "/cmd/put"
#define WOS_COMMAND_DELETE "/cmd/delete"
///@}

/** @name Misc WOS Defines
 * These defines are for various values used in the WOS rest interface
 */
///@{
#define WOS_DATE_FORMAT_STRING "date: %a, %d %b %Y %H:%M:%S GMT"
#define WOS_STATUS_LENGTH 128
#define WOS_RESOURCE_LENGTH 120
#define WOS_POLICY_LENGTH 120
#define WOS_FILE_LENGTH 256
#define WOS_DATE_LENGTH 64
#define WOS_AUTH_LENGTH 32
#define WOS_CONTENT_HEADER_LENGTH 40
#define JSON_OK 0
#define JSON_ERROR -1
///@}


/**
 * A structure to hold in memory the raw JSON results from the call to the 
 * WOS management API used to get the management statistics.
 */
typedef struct WOS_MEMORY_TYPE {
   char  *data; /**< The actual in memory data */
   size_t size; /**< data size */
} WOS_MEMORY, *WOS_MEMORY_P;

/**
 * A structure to hold the processed versions of the HTTP headers
 * returned by the execution of the libcurl operation and are generated
 * by the DDN rest interface. The ones that start with x_ddn are DDN specific.
 */
typedef struct WOS_HEADERS_TYPE {
    int  x_ddn_status; /**< Return code from the rest interface */
    int  content_length; /**< Content length as returned by rest interface */
    long x_ddn_length; /**< The length of the file */
    char x_ddn_status_string[WOS_STATUS_LENGTH]; /**< String corresponding to
                                                   the x_ddn_status */
    char *x_ddn_meta; /**< The DDN Metadata: Currently unused */
    char *x_ddn_oid; /**< The DDN object ID */
} WOS_HEADERS, *WOS_HEADERS_P;

/**
 * A structure to hold the processed versions of the json data
 * returned from the call to the DDN management API. The structure
 * fields map directly to the json returned by the call.
 */
typedef struct WOS_STATISTICS_TYPE {
   int    totalNodes;
   int    activeNodes;
   int    disconnected;
   int    clients;
   int    objectCount;
   int    rawObjectCount;
   double usableCapacity; /**< in Gigabytes */
   double capacityUsed; /**< in Gigabytes */
   char  *data;   /**< used for error reporting */
} WOS_STATISTICS, *WOS_STATISTICS_P;

extern "C" {

int 
putTheFile (const char *resource, const char *policy, const char *file, WOS_HEADERS_P headerP);
int 
getTheFile (const char *resource, const char *file, const char *destination, int mode,
            WOS_HEADERS_P headerP);
int 
getTheFileStatus (const char *resource, const char *file, WOS_HEADERS_P headerP);
int 
deleteTheFile (const char *resource, const char *file, WOS_HEADERS_P headerP);
int
getTheManagementData(const char *resource, const char *user, const char *password, 
                     WOS_STATISTICS_P statsP);

}; // extern "C" 
#endif
