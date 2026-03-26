#include "irods/hash_strategy_utilities.hpp"

#include "irods/HashStrategy.hpp"
#include "irods/base64.hpp"
#include "irods/hash_types.hpp"
#include "irods/irods_error.hpp"
#include "irods/rodsErrorTable.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <utility>

#include <fmt/format.h>

namespace
{
    auto prepare_base64_encoded_string(const irods::HashStrategy& _strategy,
                                       const irods::hash::options& _options,
                                       std::span<unsigned char> _digest_buffer,
                                       std::string& _out) -> irods::error
    {
        // LONG_CHKSUM_LEN is used here to ensure that the output buffer is large enough to hold supported hash scheme
        // outputs. For instance, CHKSUM_LEN is insufficient for a base64-encoded SHA512 hash.
        constexpr auto max_out_buffer_length = LONG_CHKSUM_LEN;

        unsigned long out_len = static_cast<unsigned long>(max_out_buffer_length); // NOLINT(google-runtime-int)
        if (!_options.include_checksum_prefix) {
            out_len -= _strategy.checksum_prefix().size();
        }

        std::array<unsigned char, max_out_buffer_length> out_buffer{};
        const auto base64_encode_ec =
            irods::base64_encode(_digest_buffer.data(), _digest_buffer.size_bytes(), out_buffer.data(), &out_len);
        if (0 != base64_encode_ec) {
            auto msg = fmt::format("{}: Failed to base64 encode hash.", __func__);
            return ERROR(base64_encode_ec, std::move(msg));
        }

        std::stringstream ins;
        if (_options.include_checksum_prefix) {
            ins << _strategy.checksum_prefix();
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        ins << std::string(reinterpret_cast<char*>(out_buffer.data()), out_len);
        _out = ins.str();

        return SUCCESS();
    } // prepare_base64_encoded_string

    auto prepare_hex_string(const irods::HashStrategy& _strategy,
                            const irods::hash::options& _options,
                            std::span<unsigned char> _digest_buffer,
                            std::string& _out) -> irods::error
    {
        std::stringstream ins;
        if (_options.include_checksum_prefix) {
            ins << _strategy.checksum_prefix();
        }
        for (const auto byte : _digest_buffer) {
            ins << std::setfill('0') << std::setw(2) << std::hex << static_cast<const int>(byte);
        }
        _out = ins.str();
        return SUCCESS();
    } // prepare_hex_string
} // anonymous namespace

namespace irods::hash::detail
{
    auto prepare_output_string(const irods::HashStrategy& _strategy,
                               const irods::hash::options& _options,
                               std::span<unsigned char> _digest_buffer,
                               std::string& _out) -> irods::error
    {
        switch (_options.output_mode) {
            case irods::hash::output_mode::base64_encoded_string:
                return prepare_base64_encoded_string(_strategy, _options, _digest_buffer, _out);

            case irods::hash::output_mode::hex_string:
                [[fallthrough]];
            default:
                return prepare_hex_string(_strategy, _options, _digest_buffer, _out);
        }
        return SUCCESS();
    } // prepare_output_string
} // namespace irods::hash::detail
