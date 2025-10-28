#include "irods/CRC64NVMEStrategy.hpp"

#include "irods/base64.hpp"
#include "irods/checksum.h"
#include "irods/rodsErrorTable.h"

#include <boost/crc.hpp>

#include <array>
#include <cstdint>
#include <cstring>

namespace irods
{
    const std::string CRC64NVME_NAME("crc64nvme");

    auto CRC64NVMEStrategy::init(boost::any& _context) const -> error
    {
        // The CRC polynomial defines the feedback terms used in the CRC computation.
        // It represents the divisor polynomial in the binary polynomial division process
        // used by the CRC algorithm. Each bit set to '1' in this constant indicates a term
        // in the polynomial. The NVMe standard specifies the 64-bit polynomial:
        //   x^64 + x^4 + x^3 + x + 1  (represented here as 0xAD93D23594C93659)
        // This polynomial determines the bit mixing pattern that creates the final checksum.
        constexpr std::uint64_t crc_polynomial = 0xAD93D23594C93659;

        // The initial remainder (also known as the initial value or seed) specifies the
        // starting value loaded into the CRC register before processing any data.
        // NVMe specifies all bits set to 1 (0xFFFFFFFFFFFFFFFF).
        constexpr std::uint64_t crc_initial_remainder = 0xFFFFFFFFFFFFFFFF;

        // The final XOR value (also called the "final remainder") is XORed with the
        // CRC register after all data has been processed. NVMe also specifies all bits
        // set to 1 (0xFFFFFFFFFFFFFFFF). This step effectively inverts the CRC result,
        // providing better error-detection symmetry for certain data patterns.
        constexpr std::uint64_t crc_final_xor = 0xFFFFFFFFFFFFFFFF;

        constexpr bool crc_reflect_input = true;
        constexpr bool crc_reflect_output = true;

        // Create the CRC calculator object
        boost::crc_basic<crc_bits>* context = new boost::crc_basic<crc_bits>(
            crc_polynomial, crc_initial_remainder, crc_final_xor, crc_reflect_input, crc_reflect_output);

        _context = context;

        return SUCCESS();
    }

    auto CRC64NVMEStrategy::update(const std::string& data, boost::any& _context) const -> error
    {
        boost::crc_basic<crc_bits>* context;
        try {
            context = boost::any_cast<boost::crc_basic<crc_bits>*>(_context);
        }
        catch (const boost::bad_any_cast& e) {
            return ERROR(SYS_INVALID_INPUT_PARAM,
                         "The context sent to CRC64NVMEStrategy::update was not a boost::crc_basic<crc_bits>*");
        }

        if (nullptr == context) {
            return ERROR(INVALID_INPUT_ARGUMENT_NULL_POINTER, "A null context was sent to CRC64NVMEStrategy::update.");
        }

        context->process_bytes(data.c_str(), data.length());
        return SUCCESS();
    }

    auto CRC64NVMEStrategy::digest(std::string& messageDigest, boost::any& _context) const -> error
    {
        boost::crc_basic<crc_bits>* context;
        try {
            context = boost::any_cast<boost::crc_basic<crc_bits>*>(_context);
        }
        catch (const boost::bad_any_cast& e) {
            return ERROR(SYS_INVALID_INPUT_PARAM,
                         "The context sent to CRC64NVMEStrategy::digest was not a boost::crc_basic<crc_bits>*");
        }

        if (nullptr == context) {
            return ERROR(INVALID_INPUT_ARGUMENT_NULL_POINTER, "A null context was sent to CRC64NVMEStrategy::digest.");
        }

        std::uint64_t checksum = context->checksum();

        // Convert uint64_t to bytes (big-endian for consistency)
        std::array<unsigned char, 8> bytes;
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            bytes[7 - i] = static_cast<unsigned char>((checksum >> (i * 8)) & 0xFF);
        }

        unsigned long out_len = CHKSUM_LEN - std::strlen(CRC64NVME_CHKSUM_PREFIX);

        std::array<unsigned char, CHKSUM_LEN> out_buffer;
        int rc = base64_encode(bytes.data(), bytes.size(), out_buffer.data(), &out_len);
        if (0 != rc) {
            delete context;
            _context = nullptr;
            return ERROR(rc, fmt::format("{}: Failed to base64 encode hash.", __func__));
        }

        // The checksum has been extracted from the context.  The context can be deleted.
        delete context;
        _context = nullptr;

        messageDigest = CRC64NVME_CHKSUM_PREFIX;
        messageDigest += std::string(reinterpret_cast<char*>(out_buffer.data()), out_len);

        return SUCCESS();
    }

    bool CRC64NVMEStrategy::isChecksum(const std::string& _chksum) const
    {
        return _chksum.starts_with(CRC64NVME_CHKSUM_PREFIX);
    }
} // namespace irods
