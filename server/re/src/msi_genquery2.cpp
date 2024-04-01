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
            return SYS_INVALID_INPUT_PARAM;
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
            return SYS_INVALID_INPUT_PARAM;
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

        auto iter = gq2_context.find(ctx_handle_index);

        if (std::end(gq2_context) == iter) {
            log_msi::error("{}: Unknown context handle.", __func__);
            return SYS_INVALID_INPUT_PARAM;
        }

        gq2_context.erase(iter);

        return 0;
    }
    catch (const std::exception& e) {
        log_msi::error("{}: {}", __func__, e.what());
        return SYS_LIBRARY_ERROR;
    }
} // msi_genquery2_free
