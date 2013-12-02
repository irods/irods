#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "rodsClient.h"


int
main(int argc, char *argv[]) 
{
    char *username;
    char username2[NAME_LEN];
    char *challenge, *keybuf;
    int uid, nb, challenge_len, key_len;
    char authenticator[16];  /* hard coded at 16 bytes .. size of an md5 hash */

    /* make sure the provided username matches what the OS thinks */
    username = getenv(OS_AUTH_ENV_USER);
    if (username == NULL) {
        /* probably means someone has run from command-line */
        printf("%s is run through an iRODS library call, and probably won't do anything useful for you.\n",
               argv[0]);
        printf("%s exiting.\n", argv[0]);
        exit(1);
    }
    uid = osauthGetUsername(username2, NAME_LEN);
    if (uid == -1) {
        exit(1);
    }
    if (strcmp(username, username2)) {
        printf("Username %s does not match OS user %s",
               username, username2);
        exit(1);
    }
    
    /* read the challenge from stdin */
    nb = read(0, (void*)&challenge_len, sizeof(int));
    if (nb != sizeof(int)) {
        printf("Couldn't read challenge length from stdin: %s",
               strerror(errno));
        exit(1);
    }
    challenge = (char*)malloc(challenge_len);
    if (challenge == NULL) {
        printf("Error allocating memory for challenge: %s", strerror(errno));
        exit(1);
    }
    nb = read(0, challenge, challenge_len);
    if (nb != challenge_len) {
        printf("Couldn't read challenge from stdin: %s",
               strerror(errno));
        exit(1);
    }
    
    /* read the key from the key file */
    if (osauthGetKey(&keybuf, &key_len)) {
        printf("Error retrieving key. Exiting.");
        exit(1);
    }

    if (osauthGenerateAuthenticator(username, uid, challenge, keybuf, key_len, authenticator, 16)) {
        printf("Could not generate the authenticator");
        exit(1);
    }

    /* write out the authenticator to stdout */
    write(1, authenticator, 16);

    exit(0);
}
    
