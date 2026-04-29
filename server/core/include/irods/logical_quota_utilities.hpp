#ifndef IRODS_LOGICAL_QUOTA_UTILITIES_HPP
#define IRODS_LOGICAL_QUOTA_UTILITIES_HPP

/// \file

#include <sys/types.h>

#include <functional>
#include <type_traits>

struct RsComm;

namespace irods::logical_quotas {
    /// Enum that represents a quota violation mode.
    /// Supports bitwise operations to represent different
    /// combinations of quota types being violated.
    ///
    /// \since 5.1.0
    enum class violation : int {
        none = 0,
        bytes = 1,
        objects = 2,
    };

    template <typename T, template<typename> typename Op>
    inline T operator_impl(T _lhs, T _rhs, Op<typename std::underlying_type_t<T>> op = Op<typename std::underlying_type_t<T>>()) noexcept {
        return static_cast<T>(op(static_cast<typename std::underlying_type_t<T>>(_lhs), static_cast<typename std::underlying_type_t<T>>(_rhs)));
    }

    inline violation operator|(violation _lhs, violation _rhs) noexcept
    {
        return operator_impl<violation, std::bit_or>(_lhs, _rhs);
    }

    inline violation& operator|=(violation& _lhs, violation _rhs) noexcept
    {
        return _lhs = (_lhs | _rhs);
    }

    inline violation operator&(violation _lhs, violation _rhs) noexcept
    {
        return operator_impl<violation, std::bit_and>(_lhs, _rhs);
    }

    inline violation& operator&=(violation& _lhs, violation _rhs) noexcept
    {
        return _lhs = (_lhs & _rhs);
    }

    inline violation operator^(violation _lhs, violation _rhs) noexcept
    {
        return operator_impl<violation, std::bit_xor>(_lhs, _rhs);
    }

    inline violation& operator^=(violation& _lhs, violation _rhs) noexcept
    {
        return _lhs = (_lhs ^ _rhs);
    }

    /// \brief Check a collection for logical quota violations.
    ///
    /// This function calls rs_get_logical_quota and interprets its results. On success, it will return an irods::logical_quotas::violation corresponding to the violated quotas on the collection.
    ///
    /// \param[in] _comm A pointer to a RsComm.
    /// \param[in] _coll_name The collection name to check for quota violations.
    ///
    /// \return An integer representing an iRODS error code or an irods::logical_quotas::violation flags value.
    /// \retval <0 on failure.
    /// \retval >=0 integer valued such that the the bitwise AND (&) of the return value and a irods::logical_quotas::violation will be nonzero if the corresponding limit was violated. (i.e. (return value & irods::logical_quotas::violation::bytes) != 0 indicates byte limit was violated)
    ///
    /// \since 5.1.0
    int check_logical_quota_violation(RsComm *_rsComm, const char* _coll_name);
}

#endif // IRODS_LOGICAL_QUOTA_UTILITIES_HPP
