#ifndef IRODS_UNIT_TEST_UTILS_HPP
#define IRODS_UNIT_TEST_UTILS_HPP

#include "irods/rcMisc.h"
#include "irods/rodsClient.h"
#include "irods/dataObjRepl.h"
#include "irods/resource_administration.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/filesystem/path.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/irods_configuration_keywords.hpp"

#include <boost/filesystem.hpp>

#include <unistd.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <fstream>
#include <iostream>

namespace unit_test_utils
{
    inline auto close_replica(RcComm& _comm, const int _fd) -> int
    {
        const auto close_input = nlohmann::json{
            {"fd", _fd},
            {"update_size", false},
            {"update_status", false},
            {"compute_checksum", false},
            {"send_notifications", false},
            {"preserve_replica_state_table", true}
        }.dump();

        return rc_replica_close(&_comm, close_input.data());
    } // close_replica

    inline auto get_hostname() noexcept -> std::string
    {
        char hostname[HOST_NAME_MAX] {};
        gethostname(hostname, sizeof(hostname));
        return hostname;
    }

    inline auto create_resource_vault(const std::string_view _vault_name) -> std::string
    {
        namespace fs = boost::filesystem;

        const auto suffix = "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        const auto vault = boost::filesystem::temp_directory_path() / (_vault_name.data() + suffix).data();

        // Create the vault for the resource and allow the iRODS server to
        // read and write to the vault.

        if (!exists(vault)) {
            fs::create_directory(vault);
        }

        fs::permissions(vault, fs::perms::add_perms | fs::perms::others_read | fs::perms::others_write);

        return vault.c_str();
    }

    inline auto add_ufs_resource(RcComm& _comm,
                                 const std::string_view _resc_name,
                                 const std::string_view _vault_name) -> void
    {
        namespace adm = irods::experimental::administration;

        const auto host_name = get_hostname();
        const auto vault_path = create_resource_vault(_vault_name);

        // The new resource's information.
        adm::resource_registration_info ufs_info;
        ufs_info.resource_name = _resc_name.data();
        ufs_info.resource_type = adm::resource_type::unixfilesystem;
        ufs_info.host_name = host_name;
        ufs_info.vault_path = vault_path;

        adm::client::add_resource(_comm, ufs_info);
    }

    inline auto replicate_data_object(RcComm& _comm,
                                      const std::string_view _path,
                                      const std::string_view _resc_name) -> bool
    {
        dataObjInp_t repl_input{};
        irods::at_scope_exit free_memory{[&repl_input] { clearKeyVal(&repl_input.condInput); }};
        std::strncpy(repl_input.objPath, _path.data(), _path.size());
        addKeyVal(&repl_input.condInput, DEST_RESC_NAME_KW, _resc_name.data());

        return rcDataObjRepl(&_comm, &repl_input) == 0;
    }

    inline auto create_empty_replica(RcComm& _comm,
                                     const irods::experimental::filesystem::path& _path,
                                     const std::string_view _resource_hierarchy = "") -> bool
    {
        const auto path_str = _path.c_str();

        dataObjInp_t open_inp{};
        irods::at_scope_exit free_open_inp{[&open_inp] { clearKeyVal(&open_inp.condInput); }};
        std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", path_str);
        open_inp.openFlags = O_CREAT | O_TRUNC | O_WRONLY;

        if (!_resource_hierarchy.empty()) {
            irods::experimental::key_value_proxy{open_inp.condInput}[RESC_HIER_STR_KW] = _resource_hierarchy;
        }

        if (const auto fd = rcDataObjOpen(&_comm, &open_inp); fd > 2) {
            openedDataObjInp_t close_inp{};
            irods::at_scope_exit free_close_inp{[&close_inp] { clearKeyVal(&close_inp.condInput); }};
            close_inp.l1descInx = fd;
            return rcDataObjClose(&_comm, &close_inp) == 0;
        }

        return false;
    } // create_empty_replica

    inline auto create_local_file(const std::string_view _path,
                                  std::size_t _size_in_bytes,
                                  char _ch) -> bool
    {
        std::ofstream out{_path.data()};

        if (!out) {
            return false; 
        }

        for (std::size_t i = 0; i < _size_in_bytes; ++i) {
            out.write(&_ch, 1);
        }

        return true;
    }

    inline auto get_agent_pid(RcComm& _comm) -> int
    {
        ExecMyRuleInp inp{};
        const auto free_cond_input = irods::at_scope_exit{[&inp] { clearKeyVal(&inp.condInput); }};
        auto cond_input = irods::experimental::make_key_value_proxy(inp.condInput);

        MsParamArray *out_array = nullptr;
        const auto clear_ms_param_array = irods::at_scope_exit{[&out_array] { clearMsParamArray(out_array, true); }};

        cond_input[irods::KW_CFG_INSTANCE_NAME] = "irods_rule_engine_plugin-irods_rule_language-instance";

        const auto out_var = std::string{"*pid"};
        const auto rule_text = fmt::format("msi_get_agent_pid({});", out_var);
        std::snprintf(inp.myRule, META_STR_LEN, "@external rule { %s }", rule_text.data());
        std::snprintf(inp.outParamDesc, LONG_NAME_LEN, "%s", out_var.data());

        if (const auto ec = rcExecMyRule(&_comm, &inp, &out_array); ec < 0) {
            return ec;
        }

        if (!out_array) {
            return -1;
        }

        try {
            return std::stoi(static_cast<char*>(out_array->msParam[0]->inOutStruct));
        }
        catch (const std::invalid_argument&) {
            return -2;
        }
        catch (const std::out_of_range&) {
            return -3;
        }
    } // get_agent_pid
} // namespace unit_test_utils

#endif // IRODS_UNIT_TEST_UTILS_HPP

