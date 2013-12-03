#ifndef CURL_WOS_HPP
#define CURL_WOS_HPP
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
#include "curlWosFunctions.h"

/** @name WOS user operations.
 * The set of supported operations. These map to the --operation user parameter.
 */
///@{
enum WOS_OPERATION_TYPE {
   WOS_PUT,
   WOS_DELETE,
   WOS_STATUS,
   WOS_FILESTATUS,
   WOS_GET
} WOS_OP;
///@}

/**
 * A structure to hold the parsed user arguments
 */
typedef struct WOS_ARG_TYPE {
    char   resource[WOS_RESOURCE_LENGTH]; /**< the http addr for the wos unit*/
    char   policy[WOS_POLICY_LENGTH]; /**< the policy for the file: put only*/
    char   file[WOS_FILE_LENGTH]; /**< The OID for get/delete. 
                                       Otherwise the file name*/
    char   destination[WOS_FILE_LENGTH]; /**< The path into which to store the 
                                              file.  get only */
    enum   WOS_OPERATION_TYPE op; /**< The type of the operation */
    char   user[WOS_AUTH_LENGTH]; /**< user name for the admin ops */
    char   password[WOS_AUTH_LENGTH]; /**< password for the admin ops */
} WOS_ARG, *WOS_ARG_P;

#endif
