/*
  This program does a PAM authentication check using the username on
  the command line and reading the password from stdin (to be more
  secure).  When PAM mode is enabled and users request LDAP/PAM
  authentication (irodsAuthScheme of 'PAM' or 'pam'), the irodsAgent
  spawns this process and writes the input password on stdin.  You can
  also run this manually, entering the password after PamAuthCheck is
  started; which will be echoed:
  $ ./PamAuthCheck testuser2 
  asfkskdlfkd
  Authenticated
  $

  You may need to install PAM libraries, such as libpam0g-dev:
  sudo apt-get install libpam0g-dev

  This is built when PAM is enabled (in config/config.mk, change 
  # PAM_AUTH = 1
  to
  PAM_AUTH = 1

  But you can also build it via:
  gcc PamAuthCheck.c -L /usr/lib -l pam -o PamAuthCheck

  It needs to be set UID root:
    sudo chown root PamAuthCheck
    sudo chmod u+s PamAuthCheck

  You may need to add the following (or equivalent) to the /etc/pam.conf file.
  # check authorization
  check   auth       required     pam_unix_auth.so
  check   account    required     pam_unix_acct.so

  Loosely based on various PAM examples.
*/

#include <security/pam_appl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char pam_service[] = "irods";
struct pam_response *reply;

int 
null_conv(int num_msg, const struct pam_message **msg, 
	  struct pam_response **resp, void *appdata_ptr) {
  *resp = reply;
  return PAM_SUCCESS;
}
 
int main(int argc, char *argv[])
{
    pam_handle_t *pamh=NULL;
    int retval;
    int nb;
    int debug=0;

    static char password[500];

    static char *username="nobody";
    static struct pam_conv conv = { null_conv, NULL };
 
    if(argc == 2) {
	username = argv[1];
    }
    else {
	fprintf(stderr, "Usage: PamAuthCheck username\n");
	exit(2);
    }

    /* read the pw from stdin */
    nb = read(0, (void*)&password, sizeof(password));
    if (debug>0) printf("nb=%d\n",nb);
    if (password[nb-1]=='\n') password[nb-1]='\0';

    retval = pam_start(pam_service, username, &conv, &pamh);
    if (debug>0) printf("retval 1=%d\n",retval);

    if (retval != PAM_SUCCESS) {
	fprintf(stderr, "PamAuthCheck: pam_start error\n");
        exit(3);
    }

    reply = (struct pam_response*)malloc(sizeof(struct pam_response));
    if (reply == NULL) {
	fprintf(stderr, "PamAuthCheck: malloc error\n");
        exit(4);
    }
    
    reply[0].resp = strdup(password);
    reply[0].resp_retcode = 0;
    	
    retval = pam_authenticate(pamh, 0);    /* check username-password */
    if (debug>0) printf("retval 2=%d\n",retval);

    strcpy(password, "                    ");

    if (retval == PAM_SUCCESS) {
	fprintf(stdout, "Authenticated\n");
    } else {
	fprintf(stdout, "Not Authenticated\n");
    }

    if (pam_end(pamh,retval) != PAM_SUCCESS) {   /* close Linux-PAM */
	pamh = NULL;
	fprintf(stderr, "PamAuthCheck: failed to release authenticator\n");
	exit(5);
    }

    return ( retval == PAM_SUCCESS ? 0:1 );   /* indicate success (valid
						 username and password) or
						 not */
}
