#ifndef _ADLER32_STRATEGY_HPP_
#define _ADLER32_STRATEGY_HPP_

/// \file

#include "irods/HashStrategy.hpp"
#include "irods/hash_types.hpp"

#include <string_view>

namespace irods {
    extern const std::string ADLER32_NAME;
    class ADLER32Strategy : public HashStrategy {
        public:
            ADLER32Strategy() {};
            virtual ~ADLER32Strategy() {};

            std::string name() const override {
                return ADLER32_NAME;
            }
            error init( boost::any& context ) const override;
            error update( const std::string& data, boost::any& context ) const override;
            error digest( std::string& messageDigest, boost::any& context ) const override;
            bool isChecksum( const std::string& ) const override;
            void free_context(boost::any& _context) const override;

            /// Produce a digest in string form based on the provided options.
            ///
            /// \param[in] _options \p irods::hash::options specifying how the output string should be formatted.
            /// \param[in] _context Context used to store information about the hash as it is updated.
            /// \param[out] _out The output string.
            ///
            /// \return irods::error Any errors that are encountered are held in the returned \p irods::error.
            ///
            /// \since 5.1.0
            auto digest(const hash::options& _options, boost::any& _context, std::string& _out) const -> error override;

            /// Return the checksum prefix for this strategy. Usually this is the strategy name followed by a colon.
            ///
            /// \return std::string_view The checksum prefix.
            ///
            /// \since 5.1.0
            [[nodiscard]] auto checksum_prefix() const -> std::string_view override;
    };
} // namespace irods

#endif // _ADLER32_STRATEGY_HPP_
