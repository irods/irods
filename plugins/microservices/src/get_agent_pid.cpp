/// \file

#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/rodsErrorTable.h"
#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"

#include <exception>
#include <functional>
#include <string>

namespace
{
    namespace log = irods::experimental::log;

    auto msi_impl(msParam_t* _out_pid, ruleExecInfo_t* _rei) -> int
    {
        try {
            const auto pid_str = std::to_string(getpid());
            fillStrInMsParam(_out_pid, pid_str.c_str());
            return 0;
        }
        catch (const std::exception& e) {
            log::microservice::error(e.what());
            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            log::microservice::error("An unknown error occurred while processing the request.");
            return SYS_UNKNOWN_ERROR;
        }
    } // msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& name, Function func) -> irods::ms_table_entry*
    {
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation(name, std::function<int(Args..., ruleExecInfo_t*)>(func));
        return msi;
    } // make_msi
} // anonymous namespace

extern "C"
auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<msParam_t*>("msi_get_agent_pid", msi_impl);
}

#ifdef IRODS_FOR_DOXYGEN
/// \brief Gets the pid of the agent which executes this microservice.
///
/// \param[out]    _out_pid A place to write down the agent pid.
/// \param[in,out] _rei     A ::RuleExecInfo object that is automatically handled by the
///                         rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.9
auto msi_get_agent_pid(msParam_t* _out_pid, ruleExecInfo_t* _rei) -> int;
#endif // IRODS_FOR_DOXYGEN

