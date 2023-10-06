#include "irods/irods_ms_plugin.hpp"
#include "irods/msParam.h"

#include "msi_assertion_macros.hpp"

namespace
{
    auto test_null_cases([[maybe_unused]] RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("testing msp_to_string: msp_to_string null cases")

        char* good = nullptr;
        msParam_t in{};

        auto ret = msp_to_string(nullptr, nullptr);
        IRODS_MSI_ASSERT(SYS_NULL_INPUT == ret);

        ret = msp_to_string(nullptr, &good);
        IRODS_MSI_ASSERT((SYS_NULL_INPUT == ret));

        in.inOutStruct = nullptr;
        ret = msp_to_string(&in, &good);
        IRODS_MSI_ASSERT((SYS_NULL_INPUT == ret));

        //NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
        in.inOutStruct = malloc(sizeof(int));
        //NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
        const auto cleanup = irods::at_scope_exit{[&] { free(in.inOutStruct); }};

        ret = msp_to_string(&in, &good);
        IRODS_MSI_ASSERT((SYS_NULL_INPUT == ret));

        IRODS_MSI_TEST_END
    } // test_null_cases

    auto test_bad_param_case([[maybe_unused]] RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("testing msp_to_string: msp_to_string bad param type")
        char* good = nullptr;
        msParam_t in{};
        //NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
        in.inOutStruct = malloc(sizeof(int));
        in.type = strdup(INT_MS_T);

        const auto cleanup = irods::at_scope_exit{[&] {
            //NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
            free(in.inOutStruct);
            //NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
            free(in.type);
        }};

        const auto ret = msp_to_string(&in, &good);
        IRODS_MSI_ASSERT((USER_PARAM_TYPE_ERR == ret));

        IRODS_MSI_TEST_END
    } // test_bad_param_case

    auto test_good_cases([[maybe_unused]] RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("testing msp_to_string: msp_to_string valid cases")

        msParam_t in{};
        char* out = nullptr;
        char* nullstr = strdup("null");
        char* validstr = strdup("S. tuberosum");

        in.inOutStruct = nullstr;
        in.type = strdup(STR_MS_T);

        const auto cleanup = irods::at_scope_exit{[&] {
            //NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
            free(nullstr);
            //NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
            free(in.type);
            //NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
            free(validstr);
        }};

        auto ret = msp_to_string(&in, &out);

        IRODS_MSI_ASSERT((0 == ret));
        IRODS_MSI_ASSERT(nullptr == out);

        in.inOutStruct = validstr;
        ret = msp_to_string(&in, &out);

        IRODS_MSI_ASSERT((0 == ret));
        IRODS_MSI_ASSERT((strcmp(out, "S. tuberosum") == 0));

        IRODS_MSI_TEST_END
    } // test_good_cases

    auto msi_impl([[maybe_unused]] RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_CASE(test_null_cases, _rei);
        IRODS_MSI_TEST_CASE(test_bad_param_case, _rei);
        IRODS_MSI_TEST_CASE(test_good_cases, _rei);

        return 0;
    } // msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& _name, Function _func) -> irods::ms_table_entry*
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation(_name, std::function<int(Args..., ruleExecInfo_t*)>(_func));
        return msi;
    } // make_msi
} // anonymous namespace

extern "C" auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<>("msi_test_msp_functions", msi_impl);
} // plugin_factory
