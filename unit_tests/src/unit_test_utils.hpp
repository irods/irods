#ifndef IRODS_UNIT_TEST_UTILS_HPP
#define IRODS_UNIT_TEST_UTILS_HPP

#include "rcMisc.h"
#include "rodsClient.h"
#include "dataObjRepl.h"
#include "resource_administration.hpp"
#include "irods_at_scope_exit.hpp"

#include <unistd.h>

#include <boost/filesystem.hpp>

#include <string>
#include <string_view>

namespace unit_test_utils
{
    inline auto get_hostname() noexcept -> std::string
    {
        char hostname[256] {};
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
                                 const std::string_view _vault_name) -> bool
    {
        namespace adm = irods::experimental::administration;

        const auto host_name = get_hostname();
        const auto vault_path = create_resource_vault(_vault_name);

        // The new resource's information.
        adm::resource_registration_info ufs_info;
        ufs_info.resource_name = _resc_name.data();
        ufs_info.resource_type = adm::resource_type::unixfilesystem.data();
        ufs_info.host_name = host_name;
        ufs_info.vault_path = vault_path;

        return adm::client::add_resource(_comm, ufs_info).value() == 0;
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
} // namespace unit_test_utils

#endif // IRODS_UNIT_TEST_UTILS_HPP
