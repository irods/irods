/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* base64.h - header file for base64.c
 */



#ifndef BASE64_HPP
#define BASE64_HPP

#include "rodsDef.hpp"

extern "C" {

    int base64_encode( const unsigned char *in,  unsigned long inlen,
                       unsigned char *out, unsigned long *outlen );
    int base64_decode( const unsigned char *in,  unsigned long inlen,
                       unsigned char *out, unsigned long *outlen );

}

#endif	/* BASE64_H */
