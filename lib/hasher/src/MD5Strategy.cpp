#include "irods/MD5Strategy.hpp"

#include "irods/rodsErrorTable.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <openssl/err.h>
#include <openssl/evp.h>

#include <fmt/format.h>

namespace irods {

    const std::string MD5_NAME("md5");

    auto MD5Strategy::init(boost::any& _context) const -> error
    {
        EVP_MD* message_digest = EVP_MD_fetch(nullptr, MD5_NAME.c_str(), nullptr);

        // Initialize the digest context for the derived digest implementation. Must use _ex or _ex2 to avoid
        // automatically resetting the context with EVP_MD_CTX_reset.
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        if (0 == EVP_DigestInit_ex2(context, message_digest, nullptr)) {
            EVP_MD_free(message_digest);
            EVP_MD_CTX_free(context);
            const auto ssl_error_code = ERR_get_error();
            auto msg = fmt::format("{}: Failed to initialize digest. error code: [{}]", __func__, ssl_error_code);
            return ERROR(DIGEST_INIT_FAILED, std::move(msg));
        }

        _context = context;

        EVP_MD_free(message_digest);

        return SUCCESS();
    }

    auto MD5Strategy::update(const std::string& data, boost::any& _context) const -> error
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
            auto msg = fmt::format("{}: Failed to calculate digest. error code: [{}]", __func__, ssl_error_code);
            return ERROR(DIGEST_UPDATE_FAILED, std::move(msg));
        }

        return SUCCESS();
    }

    auto MD5Strategy::digest(std::string& messageDigest, boost::any& _context) const -> error
    {
        EVP_MD_CTX* context = boost::any_cast<EVP_MD_CTX*>(_context);

        // Finally, retrieve the digest value from the context and place it into a buffer. Use the _ex function here
        // so that the digest context is not automatically cleaned up with EVP_MD_CTX_reset.
        unsigned char buffer[17];
        if (0 == EVP_DigestFinal_ex(context, buffer, nullptr)) {
            // Free the context here to ensure that no leaks occur. If the caller wants to try again, it needs to start
            // from init() again. Set the input to nullptr to ensure that the memory can no longer be accessed after
            // freeing.
            EVP_MD_CTX_free(context);
            _context = nullptr;
            const auto ssl_error_code = ERR_get_error();
            auto msg = fmt::format("{}: Failed to finalize digest. error code: [{}]", __func__, ssl_error_code);
            return ERROR(DIGEST_FINAL_FAILED, std::move(msg));
        }

        // The digest has been extracted from the context and placed in a buffer. The context is no longer needed, so
        // it is freed here. The input is set to nullptr to ensure that the memory can no longer be accessed after
        // freeing.
        EVP_MD_CTX_free(context);
        _context = nullptr;

        std::stringstream ins;
        for ( int i = 0; i < 16; ++i ) {
            ins << std::setfill( '0' ) << std::setw( 2 ) << std::hex << ( int )buffer[i];
        }
        messageDigest = ins.str();
        return SUCCESS();
    }

    bool
    MD5Strategy::isChecksum( const std::string& _chksum ) const {
        return std::string::npos == _chksum.find_first_not_of( "0123456789abcdefABCDEF" );
    }
}; //namespace irods
