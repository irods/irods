#ifndef IRODS_HASH_STRATEGY_UTILITIES_HPP
#define IRODS_HASH_STRATEGY_UTILITIES_HPP

/// \file

#include <span>
#include <string>

namespace irods
{
    class error;
    class HashStrategy;

    namespace hash
    {
        struct options;

        namespace detail
        {
            /// Prepare an output string from the bytes of a hash digest.
            ///
            /// \param[in] _strategy The \p irods::HashStrategy which produced the hash digest.
            /// \param[in] _options \p irods::hash::options specifying how the output string should be formatted.
            /// \param[in] _digest_buffer The buffer of bytes containing the hash digest.
            /// \param[out] _out The output string.
            ///
            /// \return irods::error Any errors that are encountered are held in the returned \p irods::error.
            ///
            /// \since 5.1.0
            auto prepare_output_string(const irods::HashStrategy& _strategy,
                                       const irods::hash::options& _options,
                                       std::span<unsigned char> _digest_buffer,
                                       std::string& _out) -> irods::error;
        } // namespace detail
    } // namespace hash
} // namespace irods

#endif // IRODS_HASH_STRATEGY_UTILITIES_HPP
