#include "irods/SHA512Strategy.hpp"

#include "irods/base64.hpp"
#include "irods/checksum.h"
#include "irods/rodsErrorTable.h"
#include "irods/hash_strategy_utilities.hpp"

#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <boost/algorithm/string/predicate.hpp>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include <fmt/format.h>

namespace irods {

    const std::string SHA512_NAME("sha512");

    error SHA512Strategy::init(boost::any& _context) const
    {
        EVP_MD* message_digest = EVP_MD_fetch(nullptr, SHA512_NAME.c_str(), nullptr);

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

    error SHA512Strategy::update(const std::string& data, boost::any& _context) const
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

    error SHA512Strategy::digest(std::string& _messageDigest, boost::any& _context) const
    {
        return digest(irods::hash::options{.output_mode = irods::hash::output_mode::base64_encoded_string,
                                           .include_checksum_prefix = true},
                      _context,
                      _messageDigest);
    }

    bool SHA512Strategy::isChecksum(const std::string& _chksum) const
    {
        return _chksum.starts_with(SHA512_CHKSUM_PREFIX);
    }

    auto SHA512Strategy::digest(const hash::options& _options, boost::any& _context, std::string& _out) const
        -> irods::error
    {
        EVP_MD_CTX* context = boost::any_cast<EVP_MD_CTX*>(_context);

        // Finally, retrieve the digest value from the context and place it into final_buffer. Use the _ex function here
        // so that the digest context is not automatically cleaned up with EVP_MD_CTX_reset.
        std::array<unsigned char, SHA512_DIGEST_LENGTH> final_buffer{};
        if (0 == EVP_DigestFinal_ex(context, final_buffer.data(), nullptr)) {
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

        return irods::hash::detail::prepare_output_string(*this, _options, final_buffer, _out);
    } // SHA512Strategy::digest

    auto SHA512Strategy::checksum_prefix() const -> std::string_view
    {
        return SHA512_CHKSUM_PREFIX;
    } // SHA512Strategy::checksum_prefix

    void SHA512Strategy::free_context(boost::any& _context) const
    {
        try {
            EVP_MD_CTX* context = boost::any_cast<EVP_MD_CTX*>(_context);
            EVP_MD_CTX_free(context);
            _context = nullptr;
        }
        catch (const boost::bad_any_cast& _e) {
        }
    }
}; // namespace irods
