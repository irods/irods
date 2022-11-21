#ifndef IRODS_BASE64_H
#define IRODS_BASE64_H

#warning "base64.h is deprecated and will be removed in a future iRODS release.  Please use base64.hpp."

#include "irods/rodsDef.h"

#ifdef __cplusplus
extern "C" {
#endif

int base64_encode(const unsigned char* in, unsigned long inlen, unsigned char* out, unsigned long* outlen);

int base64_decode(const unsigned char* in, unsigned long inlen, unsigned char* out, unsigned long* outlen);

#ifdef __cplusplus
}
#endif

#endif //IRODS_BASE64_H
