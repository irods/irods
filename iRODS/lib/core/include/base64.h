/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* base64.h - header file for base64.c
 */



#ifndef BASE64_H
#define BASE64_H

#include "rodsDef.h"

#ifdef  __cplusplus
extern "C" {
#endif

int base64_encode(const unsigned char *in,  unsigned long inlen,
                        unsigned char *out, unsigned long *outlen);
int base64_decode(const unsigned char *in,  unsigned long inlen,
                        unsigned char *out, unsigned long *outlen);

#ifdef  __cplusplus
}
#endif

#endif	/* BASE64_H */
