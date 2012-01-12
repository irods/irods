#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/param.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>
#include "u_signed64.h"
#include "hpss_api.h"
#include "mvr_protocol.h"
#include <sys/time.h>
#include "hpss_errno.h"
#include "hpss_String.h"

#define BUF_SIZE	1024

/* To configure your hpsstest, change HPSS_USER_ID, HPSS_KEYtAB_FILE,
 * MY_COS, WRITE_PATH and READ_PATH. MY_COS is the cos to use when creating
 * the WRITE_PATH. WRITE_PATH is the hpss path to create. READ_PATH is
 * an existing hpss file that this test tries to open for read */
#define HPSS_USER_ID	"srb"
#define HPSS_KEYtAB_FILE	"/users/u4/srb/hpssKeytab/ktb_srb"
#define MY_COS	510
#define WRITE_PATH "/users/sdsc/srb/irodsTestVault/foo1"
#define READ_PATH "/users/sdsc/srb/zero/slocal_lter_120100.tar"

int
main(int argc, char **argv)
{
    int status;
    hpss_authn_mech_t mech_type;
    hpss_cos_hints_t  HintsIn;
    hpss_cos_priorities_t HintsPri;
    int myCos;
    int destFd;
    int myFlags;
    long long mySize;
    char myBuf[BUF_SIZE];
    int srcFd;
    api_config_t      api_config;

    /* Set the authentication type to use
     */

    status = hpss_GetConfiguration(&api_config);
    if(status != 0) {
      exit( status );
    }

    api_config.AuthnMech = hpss_authn_mech_unix;
    api_config.Flags |= API_USE_CONFIG;

    status = hpss_SetConfiguration(&api_config);
    if(status != 0) {
      exit( status );
    }

    status = hpss_AuthnMechTypeFromString("unix", &mech_type);
    if(status != 0) {
	fprintf (stderr,
          "hpss_AuthnMechTypeFromString type unix failed, status = %d", 
	  status);
	exit (1);
    }

#if 0
    mech_type = hpss_authn_mech_unix;
#endif
    status = hpss_SetLoginCred(HPSS_USER_ID, mech_type, hpss_rpc_cred_client,
      hpss_rpc_auth_type_keytab, HPSS_KEYtAB_FILE);
    if(status != 0) {
	fprintf (stderr,
          "hpss_SetLoginCred ,status = %dn", status);
	exit (2);
    }

    /* open for read */
    srcFd = hpss_Open (READ_PATH, O_RDONLY, 0, NULL, NULL, NULL);
   if (srcFd < 0) {
        fprintf (stderr,
         "hpss_Open for read error, status = %d\n", srcFd);
        exit (3);
    } else {
        fprintf (stdout,
         "hpss_Open for read successflly, srcFd = %d\n", srcFd);
    }
    hpss_Close (srcFd);


    /* create WRITE_PATH */ 
    myFlags = O_CREAT | O_TRUNC | O_RDWR;
    (void) hpss_Umask((mode_t) 0000);
    myCos = MY_COS;

    memset(&HintsIn,  0, sizeof HintsIn);
    memset(&HintsPri, 0, sizeof HintsPri);
    mySize = BUF_SIZE;
    CONVERT_LONGLONG_TO_U64(mySize, HintsIn.MinFileSize);
    HintsIn.MaxFileSize = HintsIn.MinFileSize;
    HintsPri.MaxFileSizePriority = HintsPri.MinFileSizePriority =
     REQUIRED_PRIORITY;
    destFd = hpss_Open (WRITE_PATH, myFlags, 0600, &HintsIn,
      &HintsPri, NULL);

   if (destFd < 0) {
	fprintf (stderr,
         "hpss_Open error, status = %d\n", destFd);
	exit (3);
    } else {
        fprintf (stdout,
         "hpss_Open for write successflly, destFd = %d\n", destFd);
    }

    status = hpss_Write (destFd, myBuf, BUF_SIZE);
    if(status != 0) {
        fprintf (stderr,
          "hpss_Write ,status = %dn", status);
        exit (4);
    }

    hpss_Close (destFd);

    exit (0);
}
