/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* definitions for obf routines */

#ifdef  __cplusplus
extern "C" {
#endif

void obfDecodeByKey(char *in, char *key, char *out);
int obfRmPw(int opt);
int obfGetPw(char *pw);
int obfSavePw(int promptOpt, int fileOpt, int printOpt, char *pwArg);
int obfTempOps(int tmpOpt);
int obfiGetEnvKey();
int obfiGetTv(char *fileName);
int obfiDecode(char *in, char *out, int extra);
int obfiGetPw(char *fileName, char *pw);
int obfiOpenOutFile(char *fileName, int fileOpt);
int obfiWritePw(int fd, char *pw);
void obfiEncode(char *in, char *out, int extra);
void obfEncodeByKey(char *in, char *key, char *out);
void obfEncodeByKeyV2(char *in, char *key, char *key2, char *out);
void obfDecodeByKeyV2(char *in, char *key, char *key2, char *out);

char *obfGetMD5Hash(char *stringToHash);
#ifdef  __cplusplus
}
#endif

