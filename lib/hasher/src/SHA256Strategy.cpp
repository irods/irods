#include "irods/SHA256Strategy.hpp"

#include "irods/base64.hpp"
#include "irods/checksum.h"
#include "irods/rodsErrorTable.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>

#include <boost/algorithm/string/predicate.hpp>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <fmt/format.h>

namespace irods {

    const std::string SHA256_NAME("sha256");

    error SHA256Strategy::init(boost::any& _context) const
    {
        EVP_MD* message_digest = EVP_MD_fetch(nullptr, SHA256_NAME.c_str(), nullptr);

        // Initialize the digest context for the derived digest implementation. Must use _ex or _ex2 to avoid
        // automatically resetting the context with EVP_MD_CTX_reset.
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        if (0 == EVP_DigestInit_ex2(context, message_digest, nullptr)) {
            EVP_MD_free(message_digest);
            EVP_MD_CTX_free(context);
            const auto ssl_error_code = ERR_get_error();
            const auto msg = fmt::format("{}: Failed to initialize digest. error code: [{}]", __func__, ssl_error_code);
            return ERROR(DIGEST_INIT_FAILED, msg);
        }

        _context = context;

        EVP_MD_free(message_digest);

        return SUCCESS();
    }

    error SHA256Strategy::update(const std::string& data, boost::any& _context) const
    {
        EVP_MD_CTX* context = boost::any_cast<EVP_MD_CTX*>(_context);

        // Hash the specified buffer of bytes and store the results in the context.
        if (0 == EVP_DigestUpdate(context, data.c_str(), data.size())) {
            // Free the context here to ensure that no leaks occur. If the caller wants to try again, it needs to start
            // from init() again. Set the input to nullptr to ensure that the memory can no longer be accessed after
            // freeing.
            EVP_MD_CTX_free(context);
            _context = nullptr;
            const auto ssl_error_code = ERR_get_error();
            const auto msg = fmt::format("{}: Failed to calculate digest. error code: [{}]", __func__, ssl_error_code);
            return ERROR(DIGEST_UPDATE_FAILED, msg);
        }

        return SUCCESS();
    }

    error SHA256Strategy::digest(std::string& _messageDigest, boost::any& _context) const
    {
        EVP_MD_CTX* context = boost::any_cast<EVP_MD_CTX*>(_context);

        // Finally, retrieve the digest value from the context and place it into final_buffer. Use the _ex function here
        // so that the digest context is not automatically cleaned up with EVP_MD_CTX_reset.
        unsigned char final_buffer[SHA256_DIGEST_LENGTH];
        if (0 == EVP_DigestFinal_ex(context, final_buffer, nullptr)) {
            // Free the context here to ensure that no leaks occur. If the caller wants to try again, it needs to start
            // from init() again. Set the input to nullptr to ensure that the memory can no longer be accessed after
            // freeing.
            EVP_MD_CTX_free(context);
            _context = nullptr;
            const auto ssl_error_code = ERR_get_error();
            const auto msg = fmt::format("{}: Failed to finalize digest. error code: [{}]", __func__, ssl_error_code);
            return ERROR(DIGEST_FINAL_FAILED, msg);
        }

        // The digest has been extracted from the context and placed in a buffer. The context is no longer needed, so
        // it is freed here. The input is set to nullptr to ensure that the memory can no longer be accessed after
        // freeing.
        EVP_MD_CTX_free(context);
        _context = nullptr;

        int len = strlen( SHA256_CHKSUM_PREFIX );
        unsigned long out_len = CHKSUM_LEN - len;

        unsigned char out_buffer[CHKSUM_LEN];
        base64_encode(final_buffer, SHA256_DIGEST_LENGTH, out_buffer, &out_len);

        _messageDigest = SHA256_CHKSUM_PREFIX;
        _messageDigest += std::string( ( char* )out_buffer, out_len );

        return SUCCESS();
    }

    bool
    SHA256Strategy::isChecksum( const std::string& _chksum ) const {
        return boost::starts_with( _chksum, SHA256_CHKSUM_PREFIX );
    }
}; // namespace irods
