#include "irods/msi_genquery2.hpp"

#include "irods/irods_at_scope_exit.hpp"
#include "irods/rs_genquery2.hpp"
#include "irods/msParam.h"
#include "irods/irods_re_structs.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_state_table.h"
#include "irods/rodsError.h"
#include "irods/rodsErrorTable.h"
#include "irods/msi_preconditions.hpp"
#include "irods/irods_exception.hpp"

#include <fmt/format.h>
#include <boost/any.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <map>
#include <string>

namespace
{
    using log_msi = irods::experimental::log::microservice;

    struct genquery2_context
    {
        nlohmann::json rows;
        std::int32_t current_row = -1;
    }; // struct genquery_context

    // Holds the mappings for each GenQuery2 resultset.
    std::map<std::size_t, genquery2_context> gq2_context; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // anonymous namespace

/// Query the catalog using the GenQuery2 parser.
///
/// \warning This microservice is experimental and may change in the future.
/// \warning This microservice is NOT thread-safe.
///
/// GenQuery2 handles are only available to the agent which produced them.
///
/// The lifetime of a handle and its resultset is tied to the lifetime of the agent.
/// The value of the handle MUST NOT be viewed as having any meaning. Policy MUST NOT rely on
/// its structure for any reason as it may change across releases.
///
/// \param[in,out] _handle       The output parameter that will hold the handle to the resultset.
/// \param[in]     _query_string The GenQuery2 string to execute.
/// \param[in]     _rei          This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \see genquery2.h for features supported by GenQuery2.
///
/// \since 4.3.2
auto msi_genquery2_execute(MsParam* _handle, MsParam* _query_string, RuleExecInfo* _rei) -> int
{
    log_msi::trace(__func__);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle);
    IRODS_MSI_REQUIRE_VALID_POINTER(_query_string);
    IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle->type);
    IRODS_MSI_REQUIRE_VALID_POINTER(_query_string->type);

    IRODS_MSI_REQUIRE_TYPE(_query_string->type, STR_MS_T);

    IRODS_MSI_REQUIRE_VALID_POINTER(_query_string->inOutStruct);

    Genquery2Input input{};
    input.query_string = static_cast<char*>(_query_string->inOutStruct);

    char* results{};

    if (const auto ec = rs_genquery2(_rei->rsComm, &input, &results); ec < 0) {
        log_msi::error("{}: Error while executing GenQuery2 query [error_code=[{}]].", __func__, ec);
        return ec;
    }

    const auto key = gq2_context.size();
    const auto [ignored, inserted] =
        gq2_context.insert({key, {.rows = nlohmann::json::parse(results), .current_row = -1}});
    std::free(results); // NOLINT(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)

    if (!inserted) {
        log_msi::error("{}: Could not capture GenQuery2 results.", __func__);
        return SYS_LIBRARY_ERROR;
    }

    // Return the handle to the caller.
    fillStrInMsParam(_handle, std::to_string(key).c_str());

    return 0;
} // msi_genquery2_execute

/// Moves the cursor forward by one row.
///
/// \warning This microservice is experimental and may change in the future.
/// \warning This microservice is NOT thread-safe.
///
/// GenQuery2 handles are only available to the agent which produced them.
///
/// \param[in] _handle The GenQuery2 handle.
/// \param[in] _rei    This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval       0 If data can be read from the new row.
/// \retval -408000 If the end of the resultset has been reached.
/// \retval      <0 If an error occurred.
///
/// \b Example
/// \code{.py}
/// # A global constant representing the value which indicates the end of
/// # the resultset has been reached.
/// END_OF_RESULTSET = -408000;
///
/// iterating_over_genquery2_results()
/// {
///     # Execute a query. The results are stored in the Rule Engine Plugin.
///     msi_genquery2_execute(*handle, "select COLL_NAME, DATA_NAME order by DATA_NAME desc limit 1");
///
///     # Iterate over the results.
///     while (true) {
///         *ec = errorcode(msi_genquery2_next_row(*handle));
///
///         if (*ec < 0) {
///             if (END_OF_RESULTSET == *ec) {
///                 break;
///             }
///
///             failmsg(*ec, "Unexpected error while iterating over GenQuery2 resultset.");
///         }
///
///         msi_genquery2_column(*handle, '0', *coll_name); # Copy the COLL_NAME into *coll_name.
///         msi_genquery2_column(*handle, '1', *data_name); # Copy the DATA_NAME into *data_name.
///         writeLine("stdout", "logical path => [*coll_name/*data_name]");
///     }
///
///     # Free any resources used. This is handled for you when the agent is shut down as well.
///     msi_genquery2_free(*handle);
/// }
/// \endcode
///
/// \since 4.3.2
auto msi_genquery2_next_row(MsParam* _handle, RuleExecInfo* _rei) -> int
{
    log_msi::trace(__func__);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle);
    IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle->type);

    IRODS_MSI_REQUIRE_TYPE(_handle->type, STR_MS_T);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle->inOutStruct);

    try {
        const auto ctx_handle_index = std::stoll(static_cast<char*>(_handle->inOutStruct));

        if (ctx_handle_index < 0) {
            log_msi::error("{}: Unknown context handle.", __func__);
            return SYS_INVALID_INPUT_PARAM;
        }

        auto iter = gq2_context.find(ctx_handle_index);

        if (std::end(gq2_context) == iter) {
            log_msi::error("{}: Unknown context handle.", __func__);
        }

        auto& ctx = iter->second;

        if (ctx.current_row < static_cast<std::int32_t>(ctx.rows.size()) - 1) {
            ++ctx.current_row;
            log_msi::trace("{}: Incremented row position [{} => {}]. Returning 0.",
                           __func__,
                           ctx.current_row - 1,
                           ctx.current_row);
            return 0;
        }

        log_msi::trace("{}: Skipping increment of row position [current_row=[{}]]. Returning END_OF_RESULTSET ({}).",
                       __func__,
                       ctx.current_row,
                       END_OF_RESULTSET);

        return END_OF_RESULTSET;
    }
    catch (const std::exception& e) {
        log_msi::error("{}: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }
} // msi_genquery2_next_row

/// Reads the value of a column from a row within a GenQuery2 resultset.
///
/// \warning This microservice is experimental and may change in the future.
/// \warning This microservice is NOT thread-safe.
///
/// GenQuery2 handles are only available to the agent which produced them.
///
/// \param[in]     _handle       The GenQuery2 handle.
/// \param[in]     _column_index The index of the column to read. The index must be passed as a string.
/// \param[in,out] _column_value The variable to write the value of the column to.
/// \param[in]     _rei          This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 4.3.2
auto msi_genquery2_column(MsParam* _handle, MsParam* _column_index, MsParam* _column_value, RuleExecInfo* _rei) -> int
{
    log_msi::trace(__func__);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle);
    IRODS_MSI_REQUIRE_VALID_POINTER(_column_index);
    IRODS_MSI_REQUIRE_VALID_POINTER(_column_value);
    IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle->type);
    IRODS_MSI_REQUIRE_VALID_POINTER(_column_index->type);

    IRODS_MSI_REQUIRE_TYPE(_handle->type, STR_MS_T);
    IRODS_MSI_REQUIRE_TYPE(_column_index->type, STR_MS_T);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle->inOutStruct);
    IRODS_MSI_REQUIRE_VALID_POINTER(_column_index->inOutStruct);

    try {
        const auto ctx_handle_index = std::stoll(static_cast<char*>(_handle->inOutStruct));

        if (ctx_handle_index < 0) {
            log_msi::error("{}: Unknown context handle.", __func__);
            return SYS_INVALID_INPUT_PARAM;
        }

        auto iter = gq2_context.find(ctx_handle_index);

        if (std::end(gq2_context) == iter) {
            log_msi::error("{}: Unknown context handle.", __func__);
        }

        auto& ctx = iter->second;
        const auto column_index = std::stoll(static_cast<char*>(_column_index->inOutStruct));

        const auto& value = ctx.rows.at(ctx.current_row).at(column_index).get_ref<const std::string&>();
        log_msi::debug("{}: Column value = [{}]", __func__, value);
        fillStrInMsParam(_column_value, value.c_str());

        return 0;
    }
    catch (const std::exception& e) {
        log_msi::error("{}: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }
} // msi_genquery2_column

/// Frees all resources associated with a GenQuery2 handle.
///
/// \warning This microservice is experimental and may change in the future.
/// \warning This microservice is NOT thread-safe.
///
/// GenQuery2 handles are only available to the agent which produced them.
///
/// Users are expected to call this microservice when a GenQuery2 resultset is no longer needed.
/// Failing to follow this rule can result in memory leaks. However, the handle and the resultset
/// it is mapped to will be free'd on agent teardown.
///
/// \param[in] _handle The GenQuery2 handle.
/// \param[in] _rei    This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 4.3.2
auto msi_genquery2_free(MsParam* _handle, RuleExecInfo* _rei) -> int
{
    log_msi::trace(__func__);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle);
    IRODS_MSI_REQUIRE_VALID_POINTER(_rei);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle->type);

    IRODS_MSI_REQUIRE_TYPE(_handle->type, STR_MS_T);

    IRODS_MSI_REQUIRE_VALID_POINTER(_handle->inOutStruct);

    try {
        const auto ctx_handle_index = std::stoll(static_cast<char*>(_handle->inOutStruct));

        if (ctx_handle_index < 0) {
            log_msi::error("{}: Unknown context handle.", __func__);
            return SYS_INVALID_INPUT_PARAM;
        }

        gq2_context.erase(ctx_handle_index);

        return 0;
    }
    catch (const std::exception& e) {
        log_msi::error("{}: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }
} // msi_genquery2_free
