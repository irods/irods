/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* apiHandler.h - header file for apiHandler.h
 */



#ifndef API_HANDLER_H
#define API_HANDLER_H

#include "rods.h"
#include "sockComm.h"
#include "packStruct.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
    int apiNumber;              /* the API number */
    char *apiVersion;           /* The API version of this call */
    int clientUserAuth;         /* Client user authentication level.  
				 * NO_USER_AUTH, REMOTE_USER_AUTH, 
				 * LOCAL_USER_AUTH, REMOTE_PRIV_USER_AUTH or 
				 * LOCAL_PRIV_USER_AUTH */
    int proxyUserAuth;		/* same for proxyUser */ 
    packInstruct_t inPackInstruct;  /* the packing instruct for the input 
				     * struct */
    int inBsFlag;         	/* input bytes stream flag. 0 ==> no input 
				 * byte stream. 1 ==> we have an input byte 
				 * stream */
    packInstruct_t outPackInstruct;  /* the packing instruction for the 
				     * output struct */
    int outBsFlag;        	/* output bytes stream. 0 ==> no output byte 
				 * stream. 1 ==> we have an output byte stream 
				 */
    funcPtr svrHandler;   /* the server handler. should be defined NULL for
                           * client */

} apidef_t;

#ifdef  __cplusplus
}
#endif

#endif	/* API_HANDLER_H */
