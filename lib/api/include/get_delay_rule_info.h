#ifndef IRODS_GET_DELAY_RULE_INFO_H
#define IRODS_GET_DELAY_RULE_INFO_H

/// \file

struct RcComm;
struct BytesBuf;

#ifdef __cplusplus
extern "C" {
#endif

/// Returns information about a specific delay rule.
///
/// \param[in]     _comm    A pointer to a RcComm.
/// \param[in]     _rule_id The ID of the rule to fetch information for. It must be greater
///                         than zero.
/// \param[in,out] _output  The address to a BytesBuf pointer. On success, it will point
///                         to a JSON string containing all information about the delay rule.
///                         The string will be allocated on the heap and must be free'd by
///                         the caller using free(). On failure, the pointer is not modified.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 4.2.12
int rc_get_delay_rule_info(struct RcComm* _comm, const char* _rule_id, BytesBuf** _output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_DELAY_RULE_INFO_H
