/* wosFunctPP.hpp - header file for wosFunctPP.cpp */

#ifndef WOS_FUNCT_PP_HPP
#define WOS_FUNCT_PP_HPP
#if 0
typedef long long rodsLong_t;
#define UNIX_FILE_STAT_ERR		-516000 
#define UNIX_FILE_OPEN_ERR		-510000 
#define WOS_CONNECT_ERR		        -900000 
#define UNIX_FILE_OPEN_ERR		-510000 
#define SYS_COPY_LEN_ERR		 -27000
#define WOS_PUT_ERR			-750000
#define WOS_STREAM_PUT_ERR		-751000
#define WOS_STREAM_CLOSE_ERR		-752000
#define WOS_GET_ERR                     -753000
#define WOS_STREAM_GET_ERR              -754000
#define WOS_UNLINK_ERR              	-755000
#define WOS_STAT_ERR              	-756000
#define DEF_FILE_CREATE_MODE	0750
#define MAX_NAME_LEN	1024
#define WOS_HOST_ENV	"wosHost"
#define WOS_POLICY_ENV	"wosPolicy"
#endif

#define SRC_FILE_NAME	"/tmp/foo"
#define DEST_FILE_NAME	"/tmp/fooout"
#define WOS_HOST	"wos.renci.org"
#define WOS_POLICY	"irodstest"

/**
 * The maximum sized object which we can do a Put for.
 */
#define WSIZE_MAX (64*1024*1024-512)
#define WOS_STREAM_BUF_SIZE	(16*1024*1024)

#ifdef  __cplusplus
extern "C" {
#endif
int
wosSyncToArchPP (int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize);
int
wosStageToCachePP (int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize);
int
wosFileUnlinkPP (char *fileName);
rodsLong_t
wosGetFileSizePP (char *filename);

#ifdef  __cplusplus
}
#endif

#endif	/* WOS_FUNCT_PP_HPP */

