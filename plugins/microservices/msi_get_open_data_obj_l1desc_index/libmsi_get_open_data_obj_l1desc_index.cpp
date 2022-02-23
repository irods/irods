/// \file

#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/rodsErrorTable.h"
#include "irods/irods_error.hpp"
#include "irods/irods_log.hpp"
#include "irods/objDesc.hpp"
#include "irods/fileOpr.hpp"

#include <exception>
#include <functional>
#include <string>

// persistent L1 object descriptor table
extern l1desc_t L1desc[NUM_L1_DESC];

namespace
{
    int msi_impl(msParam_t* _in, msParam_t* _out, ruleExecInfo_t* _rei)
    {
        // check sanity
        if (!_in || !_out) {
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        // input param needs to be a string
        const char *objPath = nullptr;

        if (!(objPath = parseMspForStr(_in))) {
            return SYS_INVALID_INPUT_PARAM;
        }

        int l1descInx = -1;

        for (const l1desc_t &l1 : L1desc)
        {
            // for a valid descriptor, if the path matches...
            if (l1.inuseFlag == FD_INUSE && !strcmp(l1.dataObjInp->objPath, objPath)) {
                l1descInx = &l1 - L1desc;
                break;
            }
        }

        // error, if nothing found
        if (l1descInx == -1) {
            return NO_VALUES_FOUND;
        }

        // success, we send out the found index
        fillIntInMsParam(_out, l1descInx);
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
    return make_msi<msParam_t*, msParam_t*>("msi_get_open_data_obj_l1desc_index", msi_impl);
}

#ifdef IRODS_FOR_DOXYGEN
/// \brief Gets the L1 object descriptor index of the data object (if currently open).
///
/// \param[in]     _in      An input param of type STR_MS_T containing the object path (objPath / obj_path).
/// \param[out]    _out     An output param of type INT_MS_T containing the found L1 object descriptor index.
/// \param[in,out] _rei     A ::RuleExecInfo object that is automatically handled by the
///                         rule engine plugin framework. Users must ignore this parameter.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.9
int msi_get_open_data_obj_l1desc_index(msParam_t* _in, msParam_t* _out, ruleExecInfo_t* _rei);
#endif // IRODS_FOR_DOXYGEN
