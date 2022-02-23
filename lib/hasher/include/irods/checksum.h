#ifndef IRODS_CHECKSUM_H
#define IRODS_CHECKSUM_H

struct KeyValPair;

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_CHKSUM_PREFIX    "sha2:"
#define SHA512_CHKSUM_PREFIX    "sha512:"
#define ADLER32_CHKSUM_PREFIX   "adler32:"
#define SHA1_CHKSUM_PREFIX      "sha1:"

int verifyChksumLocFile(char *fileName, const char *myChksum, char *chksumStr);

int chksumLocFile(const char *fileName, char *chksumStr, const char* hashScheme);

int hashToStr(unsigned char *digest, char *digestStr);

int rcChksumLocFile(char *fileName,
                    char *chksumFlag,
                    struct KeyValPair *condInput,
                    const char *hashScheme);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_CHECKSUM_H
