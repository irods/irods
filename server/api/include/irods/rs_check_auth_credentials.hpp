#ifndef IRODS_RS_CHECK_AUTH_CREDENTIALS_HPP
#define IRODS_RS_CHECK_AUTH_CREDENTIALS_HPP

/// \file

#include "irods/check_auth_credentials.h"

struct RsComm;

/// Verifies the native authentication credentials passed match the credentials stored
/// in the catalog.
///
/// This API endpoint only supports native authentication and requires rodsadmin
/// privileges.
///
/// It does NOT modify any state on either side of the connection.
///
/// \param[in] _comm    A pointer to a RsComm.
/// \param[in] _input   A pointer to a CheckAuthCredentialsInput.
/// \param[in] _correct \parblock A pointer to an integer pointer used to indicate
/// whether the native authentication credentials are correct.
///
/// On success, the pointer will point to memory on the heap. The caller is
/// responsible for freeing the memory.
///
/// On failure, the pointer will be set to NULL.
///
/// If the credentials are correct, \p *_correct will be set to 1.
/// If the credentials are not correct, \p *_correct will be set to 0.
/// \endparblock
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \b Example
/// \code{.cpp}
/// RsComm* comm = // Our rodsadmin connection object.
///
/// // Configure the input object for the API call.
/// struct CheckAuthCredentialsInput input;
/// memset(&input, 0, sizeof(struct CheckAuthCredentialsInput));
///
/// strncpy(input.username, "bob", sizeof(input.username) - 1);
/// strncpy(input.zone, "tempZone", sizeof(input.zone) - 1);
///
/// char* expected_password = // What we expect bob's password to be.
/// char* scrambled_password = // expected_password, but scrambled. See #obfEncodeByKey.
/// strncpy(input.password, scrambled_password, sizeof(input.password) - 1);
///
/// int* correct = NULL;
///
/// // Verify if the given password for bob is result.
/// const int ec = rs_check_auth_credentials(comm, &input, &correct);
///
/// if (ec < 0) {
///     // Handle error.
/// }
///
/// if (correct) {
///     if (1 == *correct) {
///         // The credentials are correct.
///     }
///     else {
///         // The credentials are not correct.
///     }
///
///     // Don't forget to free the memory.
///     free(correct);
/// }
/// \endcode
///
/// \since 4.3.1
int rs_check_auth_credentials(RsComm* _comm, CheckAuthCredentialsInput* _input, int** _correct);

#endif // IRODS_RS_CHECK_AUTH_CREDENTIALS_HPP
