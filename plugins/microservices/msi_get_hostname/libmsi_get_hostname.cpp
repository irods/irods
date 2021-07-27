/// \file

#include "irods_ms_plugin.hpp"
#include "irods_re_structs.hpp"
#include "msParam.h"
#include "rodsErrorTable.h"
#include "irods_error.hpp"
#include "irods_log.hpp"
#include "fileOpr.hpp"

#include <exception>
#include <string>
#include <unistd.h>

namespace
{
    int msi_impl(msParam_t *_out, ruleExecInfo_t* _rei){
        if (!_out || !_rei){
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }
        char hostname[HOST_NAME_MAX];
        gethostname(hostname, HOST_NAME_MAX);
        fillStrInMsParam(_out, _rei->rsComm->myEnv.rodsHost);
        return 0;
    }// msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& name, Function func) -> irods::ms_table_entry*
    {
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation<Args..., ruleExecInfo_t*>(name, std::function<int(Args..., ruleExecInfo_t*)>(func));
        return msi;
    } // make_msi
}// anonymous namespace

extern "C"
auto plugin_factory() -> irods::ms_table_entry* {
    return make_msi<msParam_t*>("msi_get_hostname",msi_impl);
}

#ifdef IRODS_FOR_DOXYGEN
/// \brief Gets the hostname of the server executing the rule
///
/// \param[out] _hostname A place to write the hostname
/// \param[in,out] _rei A ::RuleExecInfo object that is automatically handled by the
///                       rule engine plugin framework. Users should ignore
/// \return An integer representing the error code
/// \since 4.3.0
auto msi_get_hostname(msParam_t* _hostname, ruleExecInfo_t* _rei) -> int;
#endif // IRODS_FOR_DOXYGEN
