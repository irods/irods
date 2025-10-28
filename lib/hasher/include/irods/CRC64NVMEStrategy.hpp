#ifndef _CRC64NVME_STRATEGY_HPP_
#define _CRC64NVME_STRATEGY_HPP_

#include "irods/HashStrategy.hpp"

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
    };
} // namespace irods

#endif // _CRC64NVME_STRATEGY_HPP_
