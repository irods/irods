#ifndef IRODS_HASH_TYPES_HPP
#define IRODS_HASH_TYPES_HPP

/// \file

namespace irods::hash
{
    /// Different modes of output for a hash digest.
    ///
    /// \since 5.1.0
    enum class output_mode
    {
        hex_string,
        base64_encoded_string
    };

    /// Options for the output string of a hash digest.
    ///
    /// \since 5.1.0
    struct options
    {
        /// The \p output_mode of the output string.
        irods::hash::output_mode output_mode;

        /// If true, include the checksum prefix at the beginning of the output string.
        bool include_checksum_prefix;
    };
} // namespace irods::hash

#endif // IRODS_HASH_TYPES_HPP
