#ifndef IRODS_ESCAPE_UTILITIES_HPP
#define IRODS_ESCAPE_UTILITIES_HPP

#include <string>

namespace irods
{
    /// Returns a copy of a string with all single quotes replaced by their hexadecimal value.
    ///
    /// \param[in] _s A string possibly containing single quotes.
    ///
    /// \returns std::string
    ///
    /// \since 4.3.3
    auto single_quotes_to_hex(const std::string& _s) -> std::string;


    /// TODO
    ///
    /// \since 4.3.3
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_SINGLE_QUOTES_TO_HEX_COMPAT(_s) \
    [t = (_s)] { \
        constexpr const auto* var = "IRODS_ENABLE_GENQUERY1_FLEX_BISON_PARSER"; \
        if (const auto* v = std::getenv(var); v && std::strcmp(v, "1") == 0) { \
            return irods::single_quotes_to_hex(t); \
        } \
        return t; \
    }()
} // namespace irods

#endif // IRODS_ESCAPE_UTILITIES_HPP
