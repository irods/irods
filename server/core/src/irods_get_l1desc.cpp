#include "irods/irods_get_l1desc.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/rsGlobalExtern.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "irods/replica_proxy.hpp"

namespace irods
{
    auto get_l1desc(const l1_index_type _index) -> l1desc_t&
    {
        return L1desc[_index];
    } // get_l1desc

    auto find_l1desc(
       const irods::experimental::filesystem::path& _logical_path,
       std::string_view _resource_hierarchy) -> l1desc_t*
    {
        const auto index_is_open = [](const l1_index_type _index)
        {
            return FD_INUSE == L1desc[_index].inuseFlag;
        };

        for (l1_index_type index = 3; index < NUM_L1_DESC && index_is_open(index); ++index) {
            auto& fd = L1desc[index];
            const auto repl = irods::experimental::replica::make_replica_proxy(*fd.dataObjInfo);

            if (repl.logical_path() == _logical_path.c_str() &&
                repl.hierarchy() == _resource_hierarchy) {
                return &fd;
            }
        }

        return {};
    } // find_l1desc
}; // namespace irods

