#include "irods/CRC64NVMEStrategy.hpp"

#include "irods/rodsErrorTable.h"
#include "irods/checksum.h"
#include "irods/base64.hpp"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <openssl/err.h>
#include <openssl/evp.h>

#include <fmt/format.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/crc.hpp>

namespace irods
{

    const std::string CRC64NVME_NAME("crc64nvme");

    auto CRC64NVMEStrategy::init(boost::any& _context) const -> error
    {
        const size_t crc_bits = 64;
        const uint64_t crc_polynomial = 0xAD93D23594C93659;
        const uint64_t crc_initial_remainder = 0xFFFFFFFFFFFFFFFF;
        const uint64_t crc_final_xor = 0xFFFFFFFFFFFFFFFF;
        const bool crc_reflect_input = true;
        const bool crc_reflect_output = true;

        // Create the CRC calculator object
        boost::crc_basic<crc_bits>* context = new boost::crc_basic<crc_bits>(
            crc_polynomial, crc_initial_remainder, crc_final_xor, crc_reflect_input, crc_reflect_output);

        _context = context;

        return SUCCESS();
    }

    auto CRC64NVMEStrategy::update(const std::string& data, boost::any& _context) const -> error
    {
        const size_t crc_bits = 64;
        boost::crc_basic<crc_bits>* context = boost::any_cast<boost::crc_basic<crc_bits>*>(_context);
        context->process_bytes(data.c_str(), data.length());
        return SUCCESS();
    }

    auto CRC64NVMEStrategy::digest(std::string& messageDigest, boost::any& _context) const -> error
    {
        const size_t crc_bits = 64;
        boost::crc_basic<crc_bits>* context = boost::any_cast<boost::crc_basic<crc_bits>*>(_context);

        uint64_t checksum = context->checksum();

        // Convert uint64_t to bytes (big-endian for consistency)
        std::array<unsigned char, 8> bytes;
        for (int i = 0; i < 8; ++i) {
            bytes[7 - i] = static_cast<unsigned char>((checksum >> (i * 8)) & 0xFF);
        }

        int len = strlen(CRC64NVME_CHKSUM_PREFIX);
        unsigned long out_len = CHKSUM_LEN - len;

        unsigned char out_buffer[CHKSUM_LEN];
        base64_encode(bytes.data(), 8, out_buffer, &out_len);

        // The checksum has been extracted from the context.  The context can be deleted.
        delete context;
        _context = nullptr;

        messageDigest = CRC64NVME_CHKSUM_PREFIX;
        messageDigest += std::string((char*) out_buffer, out_len);

        return SUCCESS();
    }

    bool CRC64NVMEStrategy::isChecksum(const std::string& _chksum) const
    {
        return boost::starts_with(_chksum, CRC64NVME_CHKSUM_PREFIX);
    }
}; //namespace irods
