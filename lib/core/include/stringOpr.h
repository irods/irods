#ifndef STRING_OPR_H__
#define STRING_OPR_H__

#include "rodsDef.h"

#ifdef __cplusplus
extern "C" {
#endif

char *rstrcpy(char *dest, const char *src, int maxLen);

char *rstrcat(char *dest, const char *src, int maxLen);

char *rstrncat(char *dest, const char *src, int srcLen, int maxLen);

int is_empty_string(const char* _s, size_t _max_size);

int is_non_empty_string(const char* _s, size_t _max_size);

int trimWS(char *s);

int rSplitStr(const char *inStr, char* outStr1, size_t maxOutLen1,
              char* outStr2, size_t maxOutLen2, char key);

int copyStrFromBuf(char **buf, char *outStr, int maxOutLen);

int isAllDigit(const char *myStr);

int splitPathByKey(const char *srcPath,
                   char *dir,
                   size_t maxDirLen,
                   char *file,
                   size_t maxFileLen,
                   char key);

int getParentPathlen(char *path);

int trimQuotes(char *s);

int checkStringForSystem(const char *inString);

int checkStringForEmailAddress(const char *inString);

#ifdef __cplusplus
}
#endif

#endif	/* STRING_OPR_H__ */
