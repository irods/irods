#include "irods/msi_genquery2.hpp"

#include "irods/irods_at_scope_exit.hpp"
#include "irods/rs_genquery2.hpp"
#include "irods/msParam.h"
#include "irods/irods_re_structs.hpp"
#include "irods/irods_logger.hpp"
//#include "irods/irods_plugin_context.hpp"
//#include "irods/irods_re_plugin.hpp"
#include "irods/irods_state_table.h"
#include "irods/rodsError.h"
#include "irods/rodsErrorTable.h"
#include "irods/msi_preconditions.hpp"

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

auto msi_genquery2_execute(MsParam* _handle, MsParam* _query_string, RuleExecInfo* _rei) -> int
{
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
        log_msi::error("Error while executing GenQuery2 query [error_code=[{}]].", ec);
        return ec;
    }

    gq2_context.push_back({.rows = nlohmann::json::parse(results), .current_row = -1});
    std::free(results); // NOLINT(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)

    // Return the handle to the caller.
    fillStrInMsParam(_handle, std::to_string(gq2_context.size() - 1).c_str());

    return 0;
} // msi_genquery2_execute

auto msi_genquery2_next_row() -> int
{
    return 0;
} // msi_genquery2_next_row

auto msi_genquery2_column() -> int
{
    return 0;
} // msi_genquery2_column

auto msi_genquery2_destroy() -> int
{
    return 0;
} // msi_genquery2_destroy
