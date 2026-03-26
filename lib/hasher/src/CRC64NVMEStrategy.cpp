#include "irods/CRC64NVMEStrategy.hpp"

#include "irods/base64.hpp"
#include "irods/checksum.h"
#include "irods/rodsErrorTable.h"
#include "irods/hash_strategy_utilities.hpp"

#include <boost/crc.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace
{
    constexpr std::uint8_t CRC64NVME_CHECKSUM_LEN = 8;
} // namespace

namespace irods
{
    const std::string CRC64NVME_NAME("crc64nvme");

    auto CRC64NVMEStrategy::init(boost::any& _context) const -> error
    {
        // The CRC polynomial defines the feedback terms used in the CRC computation.
        // It represents the divisor polynomial in the binary polynomial division process
        // used by the CRC algorithm. Each bit set to '1' in this constant indicates a term
        // in the polynomial. The NVMe standard specifies the 64-bit polynomial:
        //   x^64 + x^63 + x^61 + x^59 + x^56 + x^55 + x^52 + x^49 + x^48 + x^47 +
        //   x^46 + x^44 + x^41 + x^37 + x^36 + x^34 + x^32 + x^31 + x^28 + x^26 +
        //   x^23 + x^22 + x^19 + x^16 + x^13 + x^12 + x^10 + x^9 + x^6 + x^4 + x^3 + 1
        //   (represented here as 0xAD93D23594C93659)
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
        return digest(irods::hash::options{.output_mode = irods::hash::output_mode::base64_encoded_string,
                                           .include_checksum_prefix = true},
                      _context,
                      messageDigest);
    }

    bool CRC64NVMEStrategy::isChecksum(const std::string& _chksum) const
    {
        return _chksum.starts_with(CRC64NVME_CHKSUM_PREFIX);
    }

    auto CRC64NVMEStrategy::digest(const hash::options& _options, boost::any& _context, std::string& _out) const
        -> irods::error
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

        // The checksum has been extracted from the context.  The context can be deleted.
        delete context;
        _context = nullptr;

        // Convert uint64_t to bytes (big-endian for consistency)
        std::array<unsigned char, CRC64NVME_CHECKSUM_LEN> bytes{};
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            // NOLINTNEXTLINE(hicpp-signed-bitwise, cppcoreguidelines-pro-bounds-constant-array-index)
            bytes[bytes.size() - 1 - i] = static_cast<unsigned char>((checksum >> (i * bytes.size())) & 0xFF);
        }

        return irods::hash::detail::prepare_output_string(*this, _options, bytes, _out);
    } // CRC64NVMEStrategy::digest

    auto CRC64NVMEStrategy::checksum_prefix() const -> std::string_view
    {
        return CRC64NVME_CHKSUM_PREFIX;
    } // CRC64NVMEStrategy::checksum_prefix
} // namespace irods
