/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* checksum.hpp - Extern global declaration for client API */

#ifndef CHECKSUM_HPP_
#define CHECKSUM_HPP_

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "objInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_CHKSUM_PREFIX "sha2:"
int verifyChksumLocFile( char *fileName, const char *myChksum, char *chksumStr );

int
chksumLocFile( const char *fileName, char *chksumStr, const char* );
int
hashToStr( unsigned char *digest, char *digestStr );
int
rcChksumLocFile( char *fileName, char *chksumFlag, keyValPair_t *condInput, const char* );

#ifdef __cplusplus
}
#endif

#endif	/* CHECKSUM_HPP_ */
