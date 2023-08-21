#ifndef IRODS_GET_LIBRARY_FEATURES_H
#define IRODS_GET_LIBRARY_FEATURES_H

/// \file

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Returns a JSON string containing information about the connected server's library features.
///
/// \param[in]  _comm     A pointer to a RcComm.
/// \param[out] _features \parblock A pointer to a pointer that will hold the results.
///
/// Upon success, the output variable will contain a JSON object with the following structure.
/// \code{.js}
/// {
///     "feature_1": integer,
///     "feature_2": integer,
///     // ...
///     "feature_N": integer
/// }
/// \endcode
/// Where \p feature_X matches a C macro name defined in library_features.h. The integer value
/// of \p feature_X will match the integer value of the macro.
/// \endparblock
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.3.1
int rc_get_library_features(struct RcComm* _comm, char** _features);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_LIBRARY_FEATURES_H
