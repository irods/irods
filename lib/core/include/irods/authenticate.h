#ifndef IRODS_AUTHENTICATE_H
#define IRODS_AUTHENTICATE_H

// These macros are used extensively throughout the authentication code. Over time, we should move away from using
// these, but we cannot escape from it for now. Just know that these values are currently very important.
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define MAX_PASSWORD_LEN 50
#define CHALLENGE_LEN    64
#define RESPONSE_LEN     16
// NOLINTEND(cppcoreguidelines-macro-usage)

#endif // IRODS_AUTHENTICATE_H
