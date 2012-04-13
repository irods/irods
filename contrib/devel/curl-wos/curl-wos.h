// This code is for issue 721
#ifndef CURL_WOS_H
#define CURL_WOS_H

#define WOS_DATE_FORMAT_STRING "date: %a, %d %b %Y %H:%M:%S GMT"
#define WOS_STATUS_LENGTH 128
#define WOS_RESOURCE_LENGTH 120
#define WOS_POLICY_LENGTH 120
#define WOS_FILE_LENGTH 256
#define WOS_DATE_LENGTH 64
#define WOS_CONTENT_HEADER_LENGTH 40

// Defines for various wos header
#define WOS_CONTENT_TYPE_HEADER "content-type: application/octet-stream"
#define WOS_CONTENT_LENGTH_HEADER "content-length: 0"
#define WOS_CONTENT_LENGTH_PUT_HEADER "content-length: "
#define WOS_OID_HEADER "x-ddn-oid:"
#define WOS_STATUS_HEADER "x-ddn-status:"
#define WOS_META_HEADER "x-ddn-meta:"
#define WOS_POLICY_HEADER "x-ddn-policy:"

// These map directly to the WOS rest interface status codes.
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

// Let's define the WOS operations
#define WOS_COMMAND_GET "/cmd/get"
#define WOS_COMMAND_PUT "/cmd/put"
#define WOS_COMMAND_DELETE "/cmd/delete"

enum WOS_OPERATION_TYPE {
   WOS_PUT,
   WOS_DELETE,
   WOS_GET
} WOS_OP;


typedef struct WOS_ARG_TYPE {
    char   resource[WOS_RESOURCE_LENGTH];
    char   policy[WOS_POLICY_LENGTH];
    char   file[WOS_FILE_LENGTH];
    char   destination[WOS_FILE_LENGTH];
    enum   WOS_OPERATION_TYPE op;
} WOS_ARG, *WOS_ARG_P;

typedef struct WOS_HEADERS_TYPE {
    int  x_ddn_status;
    int  content_length;
    char x_ddn_status_string[WOS_STATUS_LENGTH];
    char *x_ddn_meta;
    char *x_ddn_oid;
} WOS_HEADERS, *WOS_HEADERS_P;
#endif
