#ifndef IRODS_HASHER_CRC64NVME_STRATEGY_HPP
#define IRODS_HASHER_CRC64NVME_STRATEGY_HPP

#include "irods/HashStrategy.hpp"

#include <boost/any.hpp>
#include <string>

namespace irods
{
    extern const std::string CRC64NVME_NAME;
    class CRC64NVMEStrategy : public HashStrategy
    {
      public:
        CRC64NVMEStrategy(){};
        virtual ~CRC64NVMEStrategy(){};

        std::string name() const override
        {
            return CRC64NVME_NAME;
        }
        error init(boost::any& context) const override;
        error update(const std::string&, boost::any& context) const override;
        error digest(std::string& messageDigest, boost::any& context) const override;
        bool isChecksum(const std::string&) const override;

      private:
        static constexpr size_t crc_bits = 64;
    };
} // namespace irods

#endif // IRODS_HASHER_CRC64NVME_STRATEGY_HPP
