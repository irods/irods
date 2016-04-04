#ifndef _GUI_PROGRESS_CALLBACK_H__
#define _GUI_PROGRESS_CALLBACK_H__

#ifdef __cplusplus
extern "C" {
#endif
typedef struct OperProgress {
    int oprType;
    int flag;
    rodsLong_t totalNumFiles;	/* total number of files */
    rodsLong_t totalFileSize;	/* total size of files for this operation */
    rodsLong_t totalNumFilesDone;  /* total number of files done */
    rodsLong_t totalFileSizeDone; /* total size of files done */
    char curFileName[MAX_NAME_LEN];   /* name of the file being worked on */
    rodsLong_t curFileSize;	/* the size of the file being worked on */
    rodsLong_t curFileSizeDone; /* number of bytes done for the current file */
} operProgress_t;

typedef void ( *guiProgressCallback )( operProgress_t *operProgress );

#ifdef __cplusplus
}
#endif
#endif
