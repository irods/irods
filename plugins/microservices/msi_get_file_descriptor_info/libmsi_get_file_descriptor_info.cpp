/// \file

#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/rodsErrorTable.h"
#include "irods/irods_error.hpp"
#include "irods/irods_log.hpp"
#include "irods/rs_get_file_descriptor_info.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <string>

namespace
{
    using json = nlohmann::json;

    int msi_impl(msParam_t* _in, msParam_t* _out, ruleExecInfo_t* _rei)
    {
        // check sanity
        if (!_in || !_out || !_rei) {
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        // we want a positive index
        int index = 0;

        if (strcmp(_in->type, INT_MS_T) || (index = parseMspForPosInt(_in)) < 0) {
            return SYS_INVALID_INPUT_PARAM;
        }

        // get file descriptor info API
        const json input{{"fd", index}};
        char *output = nullptr;

        if (int status = rs_get_file_descriptor_info(_rei->rsComm,
                                                     input.dump().c_str(),
                                                     &output); status < 0) {
            return status;
        }

        // output gets passed back to RE
        _out->type = strdup(STR_MS_T);
        _out->inOutStruct = output;

        return 0;
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
    return make_msi<msParam_t*, msParam_t*>("msi_get_file_descriptor_info", msi_impl);
}

#ifdef IRODS_FOR_DOXYGEN
/// \brief Returns structured data (JSON object) of an L1 object descriptor given its index.
///
/// \param[in]     _in      A L1 descriptor index (to say an iRODS FD) to the open data object.
/// \param[out]    _out     A JSON object containing the information about the open L1 object descriptor (iRODS FD).
/// \param[in,out] _rei     A ::RuleExecInfo object that is automatically handled by the
///                         rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.9
int msi_get_file_descriptor_info(msParam_t* _in, msParam_t* _out, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
