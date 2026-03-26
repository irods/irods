#ifndef IRODS_HASHER_CRC64NVME_STRATEGY_HPP
#define IRODS_HASHER_CRC64NVME_STRATEGY_HPP

/// \file

#include "irods/HashStrategy.hpp"
#include "irods/hash_types.hpp"
#include "irods/irods_error.hpp"

#include <boost/any.hpp>

#include <cstdlib>
#include <string>
#include <string_view>

namespace irods
{
    extern const std::string CRC64NVME_NAME;

    class CRC64NVMEStrategy : public HashStrategy
    {
      public:
        CRC64NVMEStrategy() {}
        virtual ~CRC64NVMEStrategy() {}

        std::string name() const override
        {
            return CRC64NVME_NAME;
        }
        error init(boost::any& context) const override;
        error update(const std::string&, boost::any& context) const override;
        error digest(std::string& messageDigest, boost::any& context) const override;
        bool isChecksum(const std::string&) const override;

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

      private:
        static constexpr std::size_t crc_bits = 64;
    };
} // namespace irods

#endif // IRODS_HASHER_CRC64NVME_STRATEGY_HPP
