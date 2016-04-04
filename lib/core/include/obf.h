/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* definitions for obf routines */
#ifndef OBF_H__
#define OBF_H__

#define HASH_TYPE_MD5     1
#define HASH_TYPE_SHA1    2
#define HASH_TYPE_DEFAULT 3
#define SHA1_FLAG_STRING ":::sha1"

#ifdef __cplusplus
extern "C" {
#endif

void obfDecodeByKey( const char *in, const char *key, char *out );
int obfRmPw( int opt );
int obfGetPw( char *pw );
int obfSavePw( int promptOpt, int fileOpt, int printOpt, const char *pwArg );
int obfTempOps( int tmpOpt );
int obfiGetEnvKey();
int obfiGetTv( char *fileName );
int obfiDecode( const char *in, char *out, int extra );
int obfiGetPw( const char *fileName, char *pw );
int obfiOpenOutFile( const char *fileName, int fileOpt );
int obfiWritePw( int fd, const char *pw );
void obfiEncode( const char *in, char *out, int extra );
void obfEncodeByKey( const char *in, const char *key, char *out );
void obfEncodeByKeyV2( const char *in, const char *key, const char *key2, char *out );
void obfDecodeByKeyV2( const char *in, const char *key, const char *key2, char *out );
void obfMakeOneWayHash( int hashType, unsigned const char *inBuf,
                        int inBufSize, unsigned char *outHash );
void obfSetDefaultHashType( int type );
int obfGetDefaultHashType();
char *obfGetMD5Hash( const char *stringToHash );

#ifdef __cplusplus
}
#endif

#endif // OBF_H__
