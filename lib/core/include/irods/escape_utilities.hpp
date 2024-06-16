#ifndef IRODS_ESCAPE_UTILITIES_HPP
#define IRODS_ESCAPE_UTILITIES_HPP

#include <string>

namespace irods
{
    /// Returns a copy of a string with all single quotes replaced by their hexadecimal value.
    ///
    /// \throws irods::exception If an error occurs while escaping the string.
    ///
    /// \param[in] _s A string possibly containing single quotes.
    ///
    /// \returns std::string
    ///
    /// \since 5.0.0
    auto single_quotes_to_hex(const std::string& _s) -> std::string;
} // namespace irods

#endif // IRODS_ESCAPE_UTILITIES_HPP
