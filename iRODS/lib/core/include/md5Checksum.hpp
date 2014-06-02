/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* md5Checksum.h - Extern global declaration for client API */

#ifndef MD5_CHECKSUM_HPP
#define MD5_CHECKSUM_HPP

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "rods.hpp"
#include "global.hpp"
#include "md5.hpp"
#include "sha1.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_CHKSUM_PREFIX "sha2:"
    int verifyChksumLocFile( char *fileName, char *myChksum, char *chksumStr );

    int
    chksumLocFile( char *fileName, char *chksumStr, const char* );
    int
    md5ToStr( unsigned char *digest, char *chksumStr );
    int
    hashToStr( unsigned char *digest, char *digestStr );
    int
    rcChksumLocFile( char *fileName, char *chksumFlag, keyValPair_t *condInput, const char* );

#ifdef __cplusplus
}
#endif

#endif	/* MD5_CHECKSUM_H */
