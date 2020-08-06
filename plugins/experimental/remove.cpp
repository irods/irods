
#include "parallel_filesystem_operation.hpp"

namespace irods::experimental::api {
    class recursive_remove : public parallel_filesystem_operation {
        void process_object(rcComm_t& comm, const fs::path& path, const json& req)
        {
            auto ur = get<bool>("unregister", req, false);
            auto nt = get<bool>("no_trash",   req, false);
            fscl::remove(comm, path, {nt, false, false, false, ur});
        }

        void collection_predicate(rcComm_t& comm, const json& req)
        {
            fscl::remove_all(comm, req.at("logical_path"), {false, false, false, false});
        }

        public:
        recursive_remove() : parallel_filesystem_operation("remove") {}
    }; // recursive_remove

} // namespace irods::experimental::api

extern "C"
irods::experimental::api::recursive_remove* plugin_factory(const std::string&, const std::string&) {
    return new irods::experimental::api::recursive_remove{};
}
