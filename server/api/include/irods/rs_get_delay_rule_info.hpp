#ifndef IRODS_RS_GET_DELAY_RULE_INFO_HPP
#define IRODS_RS_GET_DELAY_RULE_INFO_HPP

/// \file

struct RsComm;
struct BytesBuf;

#ifdef __cplusplus
extern "C" {
#endif

/// Returns information about a specific delay rule.
///
/// \param[in]     _comm    A pointer to a RsComm.
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
auto rs_get_delay_rule_info(RsComm* _comm, const char* _rule_id, BytesBuf** _output) -> int;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_GET_DELAY_RULE_INFO_HPP
