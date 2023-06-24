#ifndef IRODS_FULLY_QUALIFIED_USERNAME_HPP
#define IRODS_FULLY_QUALIFIED_USERNAME_HPP

/// \file

#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"

#include <string_view>

namespace irods::experimental
{
    /// A class which is designed to enforce use of fully-qualified iRODS usernames.
    ///
    /// This class should be used as a building block for other libraries and utilities.
    ///
    /// Instances of this class are readonly.
    ///
    /// \since 4.3.1
    class fully_qualified_username
    {
      public:
        /// Constructs a fully_qualified_username.
        ///
        /// \param[in] _name The part of the username preceding the pound sign.
        ///                  The string pased must be non-empty.
        /// \param[in] _zone The part of the username following the pound sign.
        ///                  The string pased must be non-empty.
        ///
        /// \throws irods::exception If an error occurs or a constraint is violated.
        ///
        /// \since 4.3.1
        fully_qualified_username(const std::string_view _name, const std::string_view _zone)
        {
            if (_name.empty()) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(SYS_INVALID_INPUT_PARAM, "Empty name not allowed.");
            }

            if (_zone.empty()) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(SYS_INVALID_INPUT_PARAM, "Empty zone not allowed.");
            }

            name_.assign(_name.data(), _name.size());
            zone_.assign(_zone.data(), _zone.size());
        } // constructor

        fully_qualified_username(const fully_qualified_username& _other) = default;
        auto operator=(const fully_qualified_username& _other) -> fully_qualified_username& = default;

        fully_qualified_username(fully_qualified_username&& _other) = default;
        auto operator=(fully_qualified_username&& _other) -> fully_qualified_username& = default;

        ~fully_qualified_username() = default;

        /// Returns the part of the username preceding the pound sign.
        ///
        /// \since 4.3.1
        [[nodiscard]] auto name() const noexcept -> const std::string&
        {
            return name_;
        } // name

        /// Returns the part of the username following the pound sign.
        ///
        /// \since 4.3.1
        [[nodiscard]] auto zone() const noexcept -> const std::string&
        {
            return zone_;
        } // zone

        /// Returns the fully-qualified username.
        ///
        /// \since 4.3.1
        [[nodiscard]] auto full_name() const -> std::string
        {
            std::string fn;

            fn.reserve(name_.size() + zone_.size() + 1);
            fn += name_;
            fn += '#';
            fn += zone_;

            return fn;
        } // full_name

      private:
        std::string name_;
        std::string zone_;
    }; // class fully_qualified_username
} // namespace irods::experimental

#endif // IRODS_FULLY_QUALIFIED_USERNAME_HPP
