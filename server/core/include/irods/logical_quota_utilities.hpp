#ifndef IRODS_LOGICAL_QUOTA_UTILITIES_HPP
#define IRODS_LOGICAL_QUOTA_UTILITIES_HPP

/// \file

#include <sys/types.h>

#include <functional>
#include <type_traits>

struct RsComm;

namespace irods {
    namespace logical_quotas {
        enum class logical_quota_violation : int {
            NONE = 0,
            BYTES = 1,
            OBJECTS = 2,
        };

        template <typename T, template<typename> typename Op>
        inline T operator_impl(T _lhs, T _rhs, Op<typename std::underlying_type_t<T>> op = Op<typename std::underlying_type_t<T>>()) noexcept {
            return static_cast<T>(op(static_cast<typename std::underlying_type_t<T>>(_lhs), static_cast<typename std::underlying_type_t<T>>(_rhs)));
        }

        inline logical_quota_violation operator|(logical_quota_violation _lhs, logical_quota_violation _rhs) noexcept
        {
            return operator_impl<logical_quota_violation, std::bit_or>(_lhs, _rhs);
        }

        inline logical_quota_violation& operator|=(logical_quota_violation& _lhs, logical_quota_violation _rhs) noexcept
        {
            return _lhs = (_lhs | _rhs);
        }

        inline logical_quota_violation operator&(logical_quota_violation _lhs, logical_quota_violation _rhs) noexcept
        {
            return operator_impl<logical_quota_violation, std::bit_and>(_lhs, _rhs);
        }

        inline logical_quota_violation& operator&=(logical_quota_violation& _lhs, logical_quota_violation _rhs) noexcept
        {
            return _lhs = (_lhs & _rhs);
        }

        inline logical_quota_violation operator^(logical_quota_violation _lhs, logical_quota_violation _rhs) noexcept
        {
            return operator_impl<logical_quota_violation, std::bit_xor>(_lhs, _rhs);
        }

        inline logical_quota_violation& operator^=(logical_quota_violation& _lhs, logical_quota_violation _rhs) noexcept
        {
            return _lhs = (_lhs ^ _rhs);
        }

        /// \brief Check a collection for logical quota violations.
        ///
        /// \param[in] _comm A pointer to a RsComm.
        /// \param[in] _coll_name The collection name to check for quota violations.
        ///
        /// This function calls rs_get_logical_quota and interprets its results. On success, it will return an irods::logical_quotas::logical_quota_violation corresponding to the violated quotas on the collection.
        ///
        /// \return An integer representing an iRODS error code or an irods::logical_quotas::logical_quota_violation flags value.
        /// \retval <0 on failure.
        /// \retval An integer >0 and valued such that the the bitwise AND (&) of the return value and a irods::logical_quotas::logical_quota_violation will be nonzero if the corresponding limit was violated. (i.e. (return value & irods::logical_quotas::logical_quota_violation::BYTES) != 0 indicates byte limit was violated)
        ///
        /// \since 5.1.0
        int check_logical_quota_violation(struct RsComm *_rsComm, const char* _coll_name);
    }
}
#endif // IRODS_LOGICAL_QUOTA_UTILITIES_HPP
