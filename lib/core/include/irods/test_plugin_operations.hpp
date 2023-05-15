#ifndef IRODS_TEST_PLUGIN_OPERATIONS_HPP
#define IRODS_TEST_PLUGIN_OPERATIONS_HPP

#include "irods/irods_lookup_table.hpp"
#include "irods/irods_string_tokenize.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

namespace irods
{
    using error_code_t = int;
    using munge_operations_t = std::map<std::string, error_code_t>;

    constexpr char* MUNGE_OPERATIONS = "munge_operations";

    /// \brief Set property munge_operations as a map of operations to error codes.
    ///
    /// \parblock
    /// The value associated with key \p MUNGE_OPERATIONS in \p _props must be a comma-delimited list of key-value
    /// pairs which are separated by a single colon. The keys are strings which match to operations names for the
    /// plugin and the values are integers that correspond to iRODS error codes. Here is an example context string:
    ///     munge_operations=resource_stat:-818000,resource_open:-169000
    /// \endparblock
    ///
    /// \param[in,out] _props Plugin property map which will hold the munge_operations property.
    ///
    /// \since 4.2.12 and 4.3.1
    static auto set_munge_operations(irods::plugin_property_map& _props) -> void
    {
        std::string munge_operations_str;
        if (const auto err = _props.get(MUNGE_OPERATIONS, munge_operations_str); !err.ok()) {
            return;
        }

        static constexpr char* delimiter = ",";
        std::vector<std::string> operation_tokens;
        irods::string_tokenize(munge_operations_str, delimiter, operation_tokens);

        if (operation_tokens.empty()) {
            _props.erase(MUNGE_OPERATIONS);
            return;
        }

        const auto split_key_value_pair = [](const std::string& _s) {
            static constexpr char* delimiter = ":";

            std::vector<std::string> token_pair;

            irods::string_tokenize(_s, delimiter, token_pair);

            if (2 != token_pair.size()) {
                throw std::invalid_argument{"Key-value pair should contain only one delimiter."};
            }

            return std::make_pair(token_pair.at(0), token_pair.at(1));
        };

        munge_operations_t operation_map;
        for (auto&& t : operation_tokens) {
            try {
                const auto [op, ec_str] = split_key_value_pair(t);
                operation_map[op] = boost::lexical_cast<error_code_t>(ec_str);
            }
            catch (const boost::bad_lexical_cast&) {
                // Just skip it if it's a bad input.
            }
            catch (const std::exception&) {
                // Just skip it if it's a bad input.
            }
        }

        // Replace the comma-delimited string with a vector of operation names for reference in the plugin operations.
        _props.set(MUNGE_OPERATIONS, operation_map);
    } // set_munge_operations

    static auto get_plugin_operation_return_code(irods::plugin_property_map& _props, const std::string& _name)
        -> error_code_t
    {
        munge_operations_t ops;

        if (!_props.get(MUNGE_OPERATIONS, ops).ok()) {
            return 0;
        }

        const auto op = ops.find(_name);

        return std::cend(ops) != op ? op->second : 0;
    } // plugin_operation_is_configured_to_fail
} // namespace irods

#endif // IRODS_TEST_PLUGIN_OPERATIONS_HPP
