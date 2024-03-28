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
#include <cstring>
#include <functional>
#include <iterator>
#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using log_msi = irods::experimental::log::microservice;

    // TODO Replace with process_stash, eventually.
    struct genquery_context
    {
        nlohmann::json rows;
        std::int32_t current_row = -1;
    }; // struct genquery_context

    std::vector<genquery_context> gq2_context;
} // anonymous namespace

/// Queries the catalog using the GenQuery2 parser.
///
/// \note This microservice is experimental and may change in the future.
///
/// \param[in,out] _handle       The output parameter that will hold the handle to the resultset.
/// \param[in]     _query_string The GenQuery2 string to execute.
/// \param[in]     _rei          This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
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

    GenQuery2Input input{};
    input.query_string = static_cast<char*>(_query_string->inOutStruct);

    char* results{};

    if (const auto ec = rs_genquery2(_rei->rsComm, &input, &results); ec < 0) {
        log_msi::error("{}: Error while executing GenQuery2 query [error_code=[{}]].", __func__, ec);
        return ec;
    }

    gq2_context.push_back({.rows = nlohmann::json::parse(results), .current_row = -1});
    std::free(results); // NOLINT(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)

    // Return the handle to the caller.
    fillStrInMsParam(_handle, std::to_string(gq2_context.size() - 1).c_str());

    return 0;
} // msi_genquery2_execute

/// Moves the cursor forward by one row.
///
/// \note This microservice is experimental and may change in the future.
///
/// \param[in] _handle The GenQuery2 handle.
/// \param[in] _rei    This parameter is special and should be ignored.
///
/// \return An integer.
/// \retval  0 If data can be read from the new row.
/// \retval <0 If the end of the resultset has been reached.
///
/// \b Example
/// \code{.py}
/// # Execute a query. The results are stored in the Rule Engine Plugin.
/// msi_genquery2_execute(*handle, "select COLL_NAME, DATA_NAME order by DATA_NAME desc limit 1");
///
/// # Iterate over the results.
/// while (errorcode(genquery2_next_row(*handle)) == 0) {
///     genquery2_column(*handle, '0', *coll_name); # Copy the COLL_NAME into *coll_name.
///     genquery2_column(*handle, '1', *data_name); # Copy the DATA_NAME into *data_name.
///     writeLine("stdout", "logical path => [*coll_name/*data_name]");
/// }
///
/// # Free any resources used. This is handled for you when the agent is shut down as well.
/// genquery2_free(*handle);
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

        if (ctx_handle_index < 0 ||
            static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
            log_msi::error("{}: Unknown context handle.", __func__);
            return SYS_INVALID_INPUT_PARAM;
        }

        auto& ctx = gq2_context.at(static_cast<decltype(gq2_context)::size_type>(ctx_handle_index));

        if (ctx.current_row < static_cast<std::int32_t>(ctx.rows.size()) - 1) {
            ++ctx.current_row;
            log_msi::trace("{}: Incremented row position [{} => {}]. Returning 0.",
                           __func__,
                           ctx.current_row - 1,
                           ctx.current_row);
            return 0;
        }

        log_msi::trace(
            "{}: Skipping increment of row position [current_row=[{}]]. Returning GENQUERY2_END_OF_RESULTSET ({}).",
            __func__,
            ctx.current_row,
            GENQUERY2_END_OF_RESULTSET);

        return GENQUERY2_END_OF_RESULTSET;
    }
    catch (const irods::exception& e) {
        log_msi::error("{}: {}", __func__, e.client_display_what());
        return static_cast<int>(e.code());
    }
    catch (const std::exception& e) {
        log_msi::error("{}: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }

    return 0;
} // msi_genquery2_next_row

/// Reads the value of a column from a row within a GenQuery2 resultset.
///
/// \note This microservice is experimental and may change in the future.
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

        if (ctx_handle_index < 0 ||
            static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
            log_msi::error("{}: Unknown context handle.", __func__);
            return SYS_INVALID_INPUT_PARAM;
        }

        auto& ctx = gq2_context.at(static_cast<decltype(gq2_context)::size_type>(ctx_handle_index));
        const auto column_index = std::stoll(static_cast<char*>(_column_index->inOutStruct));

        const auto& value = ctx.rows.at(ctx.current_row).at(column_index).get_ref<const std::string&>();
        log_msi::debug("{}: Column value = [{}]", __func__, value);
        fillStrInMsParam(_column_value, value.c_str());

        return 0;
    }
    catch (const irods::exception& e) {
        log_msi::error("{}: {}", __func__, e.client_display_what());
        return static_cast<int>(e.code());
    }
    catch (const std::exception& e) {
        log_msi::error("{}: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }

    return 0;
} // msi_genquery2_column

/// Frees all resources associated with a GenQuery2 handle.
///
/// \note This microservice is experimental and may change in the future.
///
/// Users are expected to call this microservice when use of the GenQuery2 isn't needed any longer.
/// Failing to follow this rule can result in memory leaks.
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

        if (ctx_handle_index < 0 ||
            static_cast<decltype(gq2_context)::size_type>(ctx_handle_index) >= gq2_context.size()) {
            log_msi::error("{}: Unknown context handle.", __func__);
            return SYS_INVALID_INPUT_PARAM;
        }

        auto ctx_iter = std::begin(gq2_context);
        std::advance(ctx_iter, ctx_handle_index);
        gq2_context.erase(ctx_iter);

        return 0;
    }
    catch (const irods::exception& e) {
        log_msi::error("{}: {}", __func__, e.client_display_what());
        return static_cast<int>(e.code());
    }
    catch (const std::exception& e) {
        log_msi::error("{}: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }

    return 0;
} // msi_genquery2_free
