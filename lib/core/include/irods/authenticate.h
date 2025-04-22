#ifndef IRODS_AUTHENTICATE_H
#define IRODS_AUTHENTICATE_H

/// \file

struct RcComm;

// These macros are used extensively throughout the authentication code. Over time, we should move away from using
// these, but we cannot escape from it for now. Just know that these values are currently very important.
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define MAX_PASSWORD_LEN 50
#define CHALLENGE_LEN    64
#define RESPONSE_LEN     16
// NOLINTEND(cppcoreguidelines-macro-usage)

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Authenticate a client with a connected iRODS server.
///
/// After returning successfully, the client will be authenticated with the connected iRODS server for API requests.
///
/// \param[in] _comm The iRODS communication object.
/// \param[in] _context A string representing a JSON object. \parblock
///
/// The JSON object is used in the authentication plugin framework. The inputs will differ based on the authentication
/// scheme, and not all options are recognized by all authentication schemes. The authentication scheme can be provided
/// with the "scheme" key and a value of a string containing the name of the scheme to use (e.g. "native").
///
/// The JSON object must specify the authentication scheme to use. Minimally, it should look like this:
/// \code{.js}
/// {"scheme": "<authentication scheme name>"}
/// \endcode
///
/// If the authentication scheme is not provided in \p _context, an error will be returned.
/// \endparblock
///
/// \return An integer.
/// \retval 0 On success.
/// \retval <0 iRODS error code on failure.
///
/// \usage \parblock
/// \code{.c}
/// /* Connect to the server... */
/// RcComm* comm = rcConnect(...);
/// /* If successful, authenticate the client user with this function. */
/// int error_code = rc_authenticate_client(comm, NULL);
/// /* After handling errors, make API calls as usual. */
/// \endcode
/// \endparblock
///
/// \since 5.0.0
int rc_authenticate_client(struct RcComm* _comm, const char* _context);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_AUTHENTICATE_H
