#ifndef IRODS_CHECK_AUTH_CREDENTIALS_H
#define IRODS_CHECK_AUTH_CREDENTIALS_H

/// \file

struct RcComm;

/// The input data type used to invoke #rc_check_auth_credentials.
///
/// \since 4.3.1
typedef struct CheckAuthCredentialsInput // NOLINT(modernize-use-using)
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

    /// The part of a fully-qualified iRODS username preceding the pound sign.
    ///
    /// The following requirements must be satisfied:
    /// - The length must not exceed sizeof(UserInfo::userName) - 1 bytes
    /// - It must be null-terminated
    /// - It must be non-empty
    /// - It must not include the pound sign
    /// - It must not include the zone
    ///
    /// \since 4.3.1
    char username[64];

    /// The part of a fully-qualified iRODS username following the pound sign.
    ///
    /// The following requirements must be satisfied:
    /// - The length must not exceed sizeof(UserInfo::rodsZone) - 1 bytes
    /// - It must be null-terminated
    /// - It must be non-empty
    /// - It must not include the pound sign
    /// - It must not include the username
    ///
    /// \since 4.3.1
    char zone[64];

    /// The expected obfuscated password for the user of interest.
    ///
    /// The following requirements must be satisfied:
    /// - The length must not exceed 250 - 1 bytes
    /// - It must be null-terminated
    /// - It must be non-empty
    ///
    /// \since 4.3.1
    char password[250];

    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    // NOLINTNEND(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
} checkAuthCredsInp_t;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CheckAuthCredentialsInput_PI "str username[64]; str zone[64]; str password[250];"

#ifdef __cplusplus
extern "C" {
#endif

/// Verifies the native authentication credentials passed match the credentials stored
/// in the catalog.
///
/// This API endpoint only supports native authentication and requires rodsadmin
/// privileges.
///
/// It does NOT modify any state on either side of the connection.
///
/// \param[in] _comm    A pointer to a RcComm.
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
/// RcComm* comm = // Our rodsadmin connection object.
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
/// const int ec = rc_check_auth_credentials(comm, &input, &correct);
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
int rc_check_auth_credentials(struct RcComm* _comm, struct CheckAuthCredentialsInput* _input, int** _correct);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_CHECK_AUTH_CREDENTIALS_H
