#include "irods/password_hash.hpp"

#include "irods/authenticate.h"
#include "irods/base64.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_random.hpp"
#include "irods/irods_exception.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/irods_logger.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include <openssl/params.h>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace
{
    using log_server = irods::experimental::log::server;

    class key_derivation_function
    {
      public:
        key_derivation_function() = default;

        // Move constructor and assignment - okay.
        key_derivation_function(key_derivation_function&& other) = default;
        key_derivation_function& operator=(key_derivation_function&& other) = default;

        // Copy constructor and assignment - not allowed.
        key_derivation_function(key_derivation_function& other) = delete;
        key_derivation_function& operator=(key_derivation_function& other) = delete;

        virtual ~key_derivation_function() = default;

        virtual auto derive_key(const std::string& _password, const std::string& _salt)
            -> std::vector<unsigned char> = 0;
    }; // class key_derivation_function

    class scrypt_kdf : public key_derivation_function
    {
      public:
        explicit scrypt_kdf(const nlohmann::json& _parameters)
            : kctx{}
            , work_factor{}
            , resources{}
            , parallelism{}
            , keylen{}
            , algorithm_params{}
        {
            log_server::trace("{}: params={}", __func__, _parameters.dump());

            constexpr const char* missing_parameter_msg = "{}: {} scrypt parameter is missing.";
            constexpr const char* value_error_msg = "{}: {} must be a positive 32-bit integer.";
            constexpr const char* json_error_msg =
                "{}: JSON error occurred getting value for {} scrypt parameter. error: [{}]";

            constexpr const char* keylen_key = "key_length";
            try {
                // Get the desired key length (dkLen).
                const auto keylen_iter = _parameters.find(keylen_key);
                if (keylen_iter == _parameters.end()) {
                    THROW(SYS_INVALID_INPUT_PARAM, fmt::format(missing_parameter_msg, __func__, keylen_key));
                }
                const auto keylen_input = keylen_iter->get<std::int32_t>();
                // Even though there is technically no maximum to the desired key length, a maximum of 2025 bytes is
                // declared here because the base64-encoded string produced by such a key is 2700 bytes. The maximum
                // number of characters for the database column holding the password hashes is 2700 characters. Even
                // though this class is meant to be more general purpose, we impose this limit here for now because
                // the class has not been exposed for public use outside of the context of password hashing.
                constexpr std::int32_t maximum_key_length_in_bytes = 2025;
                if (keylen_input < 1 || keylen_input > maximum_key_length_in_bytes) {
                    THROW(SYS_INVALID_INPUT_PARAM, fmt::format(value_error_msg, __func__, keylen_key));
                }
                keylen = static_cast<std::uint32_t>(keylen_input);
            }
            catch (const nlohmann::json::exception& e) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(json_error_msg, __func__, keylen_key, e.what()));
            }

            constexpr const char* n_key = "N";
            try {
                // Get the cost factor parameter (N).
                const auto n_iter = _parameters.find(n_key);
                if (n_iter == _parameters.end()) {
                    THROW(SYS_INVALID_INPUT_PARAM, fmt::format(missing_parameter_msg, __func__, n_key));
                }
                // Although this parameter is ultimately a std::uint64_t, use a std::int32_t to detect
                // overflow/underflow. The maximum value that this allows, then, is 2^31 (2147483648). This value and
                // even larger ones are allowed by the algorithm with the right combination of configuration values.
                // However, the time spent calculating the key is impractical for use with password hashing, so nobody
                // should be using a value that large anyway. The values are only enforced here insofar as the types
                // themselves are concerned.
                const auto n_input = n_iter->get<std::int32_t>();
                if (n_input < 1) {
                    THROW(SYS_INVALID_INPUT_PARAM, fmt::format(value_error_msg, __func__, n_key));
                }
                work_factor = static_cast<std::uint64_t>(n_input);
            }
            catch (const nlohmann::json::exception& e) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(json_error_msg, __func__, n_key, e.what()));
            }

            constexpr const char* r_key = "r";
            try {
                // Get the resources parameter (r).
                const auto r_iter = _parameters.find(r_key);
                if (r_iter == _parameters.end()) {
                    THROW(SYS_INVALID_INPUT_PARAM, fmt::format(missing_parameter_msg, __func__, r_key));
                }
                const auto r_input = r_iter->get<std::int32_t>();
                if (r_input < 1) {
                    THROW(SYS_INVALID_INPUT_PARAM, fmt::format(value_error_msg, __func__, r_key));
                }
                resources = static_cast<std::uint32_t>(r_input);
            }
            catch (const nlohmann::json::exception& e) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(json_error_msg, __func__, r_key, e.what()));
            }

            constexpr const char* p_key = "p";
            try {
                // Get the parallelization parameter (p).
                const auto p_iter = _parameters.find(p_key);
                if (p_iter == _parameters.end()) {
                    THROW(SYS_INVALID_INPUT_PARAM, fmt::format(missing_parameter_msg, __func__, p_key));
                }
                const auto p_input = p_iter->get<std::int32_t>();
                if (p_input < 1) {
                    THROW(SYS_INVALID_INPUT_PARAM, fmt::format(value_error_msg, __func__, p_key));
                }
                parallelism = static_cast<std::uint32_t>(p_input);
            }
            catch (const nlohmann::json::exception& e) {
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(json_error_msg, __func__, p_key, e.what()));
            }

            // Initialize the KDF context.
            reset();

            // Construct algorithm parameters using the inputs provided.
            construct_algorithm_parameters();
        } // scrypt_kdf constructor

        scrypt_kdf(std::uint64_t _work_factor,
                   std::uint32_t _resources,
                   std::uint32_t _parallelism,
                   std::uint32_t _keylen)
            : kctx{}
            , work_factor{_work_factor}
            , resources{_resources}
            , parallelism{_parallelism}
            , keylen{_keylen}
            , algorithm_params{}
        {
            // Initialize the KDF context.
            reset();
            construct_algorithm_parameters();
        } // scrypt_kdf constructor

        // Move constructor and assignment - okay.
        scrypt_kdf(scrypt_kdf&& other) = default;
        scrypt_kdf& operator=(scrypt_kdf&& other) = default;

        // Copy constructor and assignment - not allowed.
        scrypt_kdf(scrypt_kdf& other) = delete;
        scrypt_kdf& operator=(scrypt_kdf& other) = delete;

        ~scrypt_kdf() override
        {
            free_kdf_context();
        } // ~scrypt_kdf

        auto reset() -> void
        {
            // Reset the KDF context, if necessary.
            free_kdf_context();

            // Initialize KDF context.
            EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "SCRYPT", nullptr);
            kctx = EVP_KDF_CTX_new(kdf);
            EVP_KDF_free(kdf);
        } // reset

        auto derive_key(const std::string& _password, const std::string& _salt) -> std::vector<unsigned char> override
        {
            std::array<OSSL_PARAM, 3> params{};
            auto* param = params.data();

            // Construct password parameter. The string must be copied to avoid const_cast in the octet string
            // construction.
            std::vector<char> password_buf(_password.size() + 1, '\0');
            const auto clear_password_buffer =
                irods::at_scope_exit{[&password_buf] { std::ranges::fill(password_buf, '\0'); }};
            std::strncpy(password_buf.data(), _password.c_str(), _password.size());
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *param++ = OSSL_PARAM_construct_octet_string(
                OSSL_KDF_PARAM_PASSWORD, static_cast<void*>(password_buf.data()), _password.size());

            // Construct salt parameter. The string must be copied to avoid const_cast in the octet string construction.
            std::vector<char> salt_buf(_salt.size() + 1, '\0');
            std::strncpy(salt_buf.data(), _salt.c_str(), _salt.size());
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *param++ = OSSL_PARAM_construct_octet_string(
                OSSL_KDF_PARAM_SALT, static_cast<void*>(salt_buf.data()), _salt.size());

            // Complete construction of KDF parameters.
            *param = OSSL_PARAM_construct_end();

            // Merge the algorithm parameters and KDF parameters.
            auto* merged_params = OSSL_PARAM_merge(params.data(), algorithm_params.data());
            const auto free_merged_params = irods::at_scope_exit{[&merged_params] { OSSL_PARAM_free(merged_params); }};
            if (nullptr == merged_params) {
                THROW(SYS_INTERNAL_ERR, fmt::format("{}: Failed to merge OpenSSL parameters.", __func__));
            }

            log_server::trace("{}: keylen={}, N={}, r={}, p={}", __func__, keylen, work_factor, resources, parallelism);

            // Derive the key using the parameters constructed above. The result will be held in a buffer.
            std::vector<unsigned char> hash_buf(keylen, 0);
            if (EVP_KDF_derive(kctx, hash_buf.data(), keylen, merged_params) <= 0) {
                THROW(CAT_PASSWORD_ENCODING_ERROR, fmt::format("{}: Failed to derive key for password.", __func__));
            }

            // Reset the object parameters so we can derive another key with this same instantiation, if needed.
            reset();

            // Return the hash buffer.
            return hash_buf;
        } // derive_key

      private:
        // KDF context for OpenSSL.
        EVP_KDF_CTX* kctx;
        // work/cost factor (N)
        std::uint64_t work_factor;
        // resources (r)
        std::uint32_t resources;
        // parallelism (p)
        std::uint32_t parallelism;
        // desired key length (dkLen)
        std::uint32_t keylen;
        // Parameters for scrypt algorithm. 4 elements needed for N, r, p, and "end" construct.
        std::array<OSSL_PARAM, 4> algorithm_params;

        inline auto free_kdf_context() -> void
        {
            if (nullptr != kctx) {
                EVP_KDF_CTX_free(kctx);
                kctx = nullptr;
            }
        } // free_kdf_context

        auto construct_algorithm_parameters() -> void
        {
            auto* param = algorithm_params.data();

            // Construct scrypt parameters.
            // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *param++ = OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_SCRYPT_N, &work_factor);
            *param++ = OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_SCRYPT_R, &resources);
            *param++ = OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_SCRYPT_P, &parallelism);
            // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

            // Complete construction of scrypt parameters.
            *param = OSSL_PARAM_construct_end();
        } // construct_algorithm_parameters
    }; // class scrypt_kdf

    auto get_kdf(const std::string& _algorithm, const nlohmann::json& _parameters)
        -> std::unique_ptr<key_derivation_function>
    {
        if ("scrypt" == _algorithm) {
            return std::make_unique<scrypt_kdf>(_parameters);
        }

        THROW(SYS_NOT_SUPPORTED, fmt::format("{}: Hashing algorithm [{}] not supported.", __func__, _algorithm));
    } // get_kdf
} // anonymous namespace

namespace irods
{
    auto generate_salt(std::int16_t _salt_length) -> std::string
    {
        return irods::generate_random_alphanumeric_string(_salt_length);
    } // generate_salt

    auto hash_password(const std::string& _password,
                       const std::string& _salt,
                       const std::string& _algorithm,
                       const nlohmann::json& _parameters) -> std::string
    {
        // Make sure the provided strings are not empty.
        if (_password.empty() || _salt.empty()) {
            THROW(SYS_INVALID_INPUT_PARAM,
                  fmt::format("{}: Cannot derive key from password - password or salt is empty.", __func__));
        }

        // Make sure the length of the provided password does not exceed the allowed maximum.
        constexpr auto maximum_password_length = 1024 * 1024;
        if (_password.size() > maximum_password_length) {
            THROW(PASSWORD_EXCEEDS_MAX_SIZE,
                  fmt::format("{}: Cannot derive key from password - password length exceeds maximum size.", __func__));
        }

        // Salt is generated by the server and its length is not configurable. Therefore, we do not check it here.

        try {
            // Using the provided algorithm name and parameters, construct a KDF object.
            const auto kdf = get_kdf(_algorithm, _parameters);

            // ...and derive a key for the provided password and salt.
            const auto derived_key = kdf->derive_key(_password, _salt);

            // base64-encode the buffer so that it can be represented as a string. The buffer size is double the
            // input buffer so as to give more than enough space for the encoded string. Technically, it only needs
            // a little over (4/3) times the space.
            std::uint64_t out_len = derived_key.size() * 2;
            std::vector<unsigned char> out(out_len, 0);
            const auto base64_encode_err = base64_encode(derived_key.data(), derived_key.size(), out.data(), &out_len);
            if (base64_encode_err < 0) {
                THROW(base64_encode_err, fmt::format("{}: base64 encoding of digest failed.", __func__));
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            return std::string{reinterpret_cast<char*>(out.data()), out_len};
        }
        catch (const nlohmann::json::exception& e) {
            THROW(SYS_LIBRARY_ERROR, fmt::format("{}: JSON exception occurred. error: [{}]", __func__, e.what()));
        }
        catch (const irods::exception&) {
            // Re-throw iRODS exceptions so that the std::exception handler does not intercept them.
            throw;
        }
        catch (const std::exception& e) {
            THROW(SYS_LIBRARY_ERROR, fmt::format("{}: Exception occurred. error: [{}]", __func__, e.what()));
        }
    } // hash_password
} // namespace irods
